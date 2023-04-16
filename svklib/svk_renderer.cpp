#ifndef SVKLIB_RENDERER_CPP
#define SVKLIB_RENDERER_CPP

#include "svk_renderer.hpp"

namespace svklib {

renderer::renderer(instance &inst, swapchain& swap, graphics::pipeline &pipe) 
    : inst(inst), swap(swap), pipe(pipe) 
{
    // createDrawCommandBuffer();
    createSyncObjects();
}

renderer::~renderer() {
    
    // vkFreeCommandBuffers(inst.device, commandPool->get(), drawCommandBuffer.size(), drawCommandBuffer.data());
    
    // vkDestroyCommandPool(inst.device, commandPool, nullptr);
    // commandPool->returnPool();

    for (int i = 0; i < pipe.swapChain.framesInFlight; i++) {
        vkDestroySemaphore(inst.device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(inst.device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(inst.device, inFlightFence[i], nullptr);
    }

}

// void renderer::createDrawCommandBuffer() {
//     drawCommandBuffer.resize(pipe.swapChain.framesInFlight);

//     VkCommandBufferAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//     allocInfo.commandPool = commandPool->get();
//     allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//     allocInfo.commandBufferCount = (uint32_t)drawCommandBuffer.size();

//     if (vkAllocateCommandBuffers(inst.device, &allocInfo, drawCommandBuffer.data()) != VK_SUCCESS) {
//         throw std::runtime_error("failed to allocate command buffers!");
//     }
// }

void renderer::createSyncObjects() {
    imageAvailableSemaphore.resize(pipe.swapChain.framesInFlight);
    renderFinishedSemaphore.resize(pipe.swapChain.framesInFlight);
    inFlightFence.resize(pipe.swapChain.framesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (int i = 0; i < pipe.swapChain.framesInFlight; i++) {
        if (vkCreateSemaphore(inst.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
            vkCreateSemaphore(inst.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
            vkCreateFence(inst.device, &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }

    }
}

void renderer::recordDrawCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipe.renderPass;
    renderPassInfo.framebuffer = pipe.frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = pipe.swapChain.swapChainExtent;

    std::array<VkClearValue,2> clearColor{};
    clearColor[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearColor[1].depthStencil = { 1.0f, 0 };
    
    renderPassInfo.clearValueCount = clearColor.size();
    renderPassInfo.pClearValues = clearColor.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.graphicsPipeline);

    //dynamic states

    // pipe.viewports.clear();
    // pipe.buildViewport(static_cast<float>(pipe.swapChain.swapChainExtent.width),static_cast<float>(pipe.swapChain.swapChainExtent.height));
    pipe.builderInfo.viewports[0].width = static_cast<float>(pipe.swapChain.swapChainExtent.width);
    pipe.builderInfo.viewports[0].height = static_cast<float>(pipe.swapChain.swapChainExtent.height);
    vkCmdSetViewport(commandBuffer, 0, pipe.builderInfo.viewports.size(), pipe.builderInfo.viewports.data());

    // pipe.scissors.clear();
    // pipe.buildScissor({0, 0}, pipe.swapChain.swapChainExtent);
    pipe.builderInfo.scissors[0].extent = pipe.swapChain.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, pipe.builderInfo.scissors.size(), pipe.builderInfo.scissors.data());

    //bind vertex buffers
    if (pipe.vertexBuffers.size() > 0) {
        vkCmdBindVertexBuffers(commandBuffer, 0, pipe.vertexBuffers.size(), pipe.vertexBuffers.data(), pipe.vertexBufferOffsets.data());
    }

    //bind index buffer
    if (pipe.indexBufferInfo.buff != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(commandBuffer, pipe.indexBufferInfo.buff, pipe.indexBufferInfo.offset, pipe.indexBufferInfo.type);
    }

    for (uint32_t i = 0; i < pipe.descriptorSetLayouts.size(); i++) {
        vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipe.pipelineLayout,0,1,&pipe.descriptorSets[i][currentFrame],0,nullptr);
    }

    //push constant
    if (pipe.pushConstantData != nullptr) {
        vkCmdPushConstants(commandBuffer, pipe.pipelineLayout, pipe.pushConstantRange.stageFlags, pipe.pushConstantRange.offset, pipe.pushConstantRange.size, pipe.pushConstantData);
    }

    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdDrawIndexed(commandBuffer, pipe.indexBufferInfo.count, 1, 0, 0, 0);

    //end commandBuffer

    vkCmdEndRenderPass(commandBuffer);


    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void renderer::drawFrame() {
    vkWaitForFences(inst.device, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(inst.device, pipe.swapChain.swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        pipe.reCreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(inst.device, 1, &inFlightFence[currentFrame]);

    vkResetCommandBuffer(swap.commandBuffers[currentFrame], 0);
    
    if (updateUniforms != nullptr) {
        updateUniforms(currentFrame);
    }

    recordDrawCommandBuffer(swap.commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &swap.commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (inst.graphicsQueue.submit(1,&submitInfo,inFlightFence[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    // if (vkQueueSubmit(inst.graphicsQueue, 1, &submitInfo, inFlightFence[currentFrame]) != VK_SUCCESS) {
    //     throw std::runtime_error("failed to submit draw command buffer!");
    // }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { pipe.swapChain.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    // result = vkQueuePresentKHR(inst.presentQueue, &presentInfo);
    result = inst.presentQueue.present(&presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || inst.win.frameBufferResized() ) {
        pipe.reCreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % pipe.swapChain.framesInFlight;

}


} // namespace svklib


#endif // SVKLIB_RENDERER_HPP