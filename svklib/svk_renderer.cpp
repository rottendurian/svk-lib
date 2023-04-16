#ifndef SVKLIB_RENDERER_CPP
#define SVKLIB_RENDERER_CPP

#include "svk_renderer.hpp"

namespace svklib {

namespace queue {

sync::sync(instance& inst, const int count)
    : inst(inst), count(count)
{
    createSyncObjects();
}

sync::~sync()
{
    destroySyncObjects();
}

void sync::syncFrame(const int frame)
{
    vkWaitForFences(inst.device, 1, &inFlightFence[frame], VK_TRUE, UINT64_MAX);
    vkResetFences(inst.device, 1, &inFlightFence[frame]);
    
}

VkResult sync::acquireNextImage(const int frame, VkSwapchainKHR swapChain, uint32_t& imageIndex)
{
    return vkAcquireNextImageKHR(inst.device, swapChain, UINT64_MAX, imageAvailableSemaphore[frame], VK_NULL_HANDLE, &imageIndex);
}

void sync::submitFrame(const int frame, const uint32_t commandBufferCount, VkCommandBuffer* commandBuffers, instance::svkqueue& queue) 
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[frame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = commandBuffers;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[frame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (queue.submit(1,&submitInfo,inFlightFence[frame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit command buffer!");
    }
}

VkResult sync::presentFrame(const int frame, const uint32_t imageIndex, VkSwapchainKHR swapChain, instance::svkqueue &queue)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore[frame] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    return queue.present(&presentInfo);
}

void sync::createSyncObjects()
{
    buffer = malloc(sizeof(VkSemaphore) * (count*2) + sizeof(VkFence) * count);
    if (buffer == nullptr) {
        throw std::runtime_error("failed to allocate memory for synchronization objects!");
    }
    inFlightFence = (VkFence*)buffer;
    imageAvailableSemaphore = (VkSemaphore*)((char*)buffer + sizeof(VkFence) * count);
    renderFinishedSemaphore = (VkSemaphore*)((char*)buffer + sizeof(VkFence) * count + sizeof(VkSemaphore) * count);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < count; i++) {
        if (vkCreateSemaphore(inst.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
            vkCreateSemaphore(inst.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
            vkCreateFence(inst.device, &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void sync::destroySyncObjects()
{
    for (size_t i = 0; i < count; i++) {
        vkDestroySemaphore(inst.device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(inst.device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(inst.device, inFlightFence[i], nullptr);
    }
    free(buffer);
}

} // namespace queue


renderer::renderer(instance &inst, swapchain& swap, graphics::pipeline &pipe) 
    : inst(inst), swap(swap), pipe(pipe), sync(inst, swap.swapChainImages.size())
{}

renderer::~renderer() = default;

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
    sync.syncFrame(currentFrame);

    uint32_t imageIndex;
    
    VkResult result = sync.acquireNextImage(currentFrame,swap.swapChain,imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        pipe.reCreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }


    vkResetCommandBuffer(swap.commandBuffers[currentFrame], 0);
    
    if (updateUniforms != nullptr) {
        updateUniforms(currentFrame);
    }

    recordDrawCommandBuffer(swap.commandBuffers[currentFrame], imageIndex);

    sync.submitFrame(currentFrame,1,&swap.commandBuffers[currentFrame],inst.graphicsQueue);

    result = sync.presentFrame(currentFrame,imageIndex,swap.swapChain,inst.graphicsQueue); //since my graphics queue is the same as the present queue(sync)

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || inst.win.frameBufferResized() ) {
        pipe.reCreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % pipe.swapChain.framesInFlight;

}

} // namespace svklib

#endif // SVKLIB_RENDERER_HPP