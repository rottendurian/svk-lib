#ifndef SVKLIB_PIPELINE_CPP
#define SVKLIB_PIPELINE_CPP

#include "svk_pipeline.hpp"
#include "svk_descriptor.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace svklib {

namespace graphics {

pipeline::pipeline(instance& inst,swapchain& swapChain, BuildInfo* builderInfo, 
                 VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipeline graphicsPipeline)
    : inst(inst),swapChain(swapChain),builderInfo(builderInfo),pipelineLayout(pipelineLayout),
      renderPass(renderPass),graphicsPipeline(graphicsPipeline)
{
    createDepthResources();
    createColorResources();
    createFramebuffers();
}

pipeline::~pipeline() {
    
    destroyFramebuffers();
    vkDestroyPipeline(inst.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(inst.device, pipelineLayout, nullptr);
    vkDestroyRenderPass(inst.device, renderPass, nullptr);
}

// builder

pipeline::builder pipeline::builder::begin(instance& inst, swapchain& swapChain) {
    return builder(inst,swapChain);
}

pipeline::builder::builder(instance& inst, swapchain& swapChain) 
    :inst(inst),swapChain(swapChain),threadPool(threadpool::get_instance()),info(new BuildInfo{})
{
   //TODO check if BuildInfo is nulled out 
}

//pipeline::builder::~builder() = default; 

void pipeline::builder::addToBuildQueue(std::function<void()> func)
{
    pipelineBuildQueue.emplace_back(false);
    threadPool->add_task(func,&pipelineBuildQueue.back());
}

void pipeline::updatePushConstantData(void* data) {
    pushConstantData = data;
}

pipeline::builder& pipeline::builder::buildPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size) {
    info->pushConstantRange = {stageFlags,offset,size};
    return *this;
}

pipeline::builder& pipeline::builder::buildDepthStencil()
{
    //addToBuildQueue([this](){
        // VkPipelineDepthStencilStateCreateInfo depthStencil{};
        info->depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        info->depthStencil.depthTestEnable = VK_TRUE;
        info->depthStencil.depthWriteEnable = VK_TRUE;
        info->depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        info->depthStencil.depthBoundsTestEnable = VK_FALSE;
        info->depthStencil.minDepthBounds = 0.0f; // Optional
        info->depthStencil.maxDepthBounds = 1.0f; // Optional
        info->depthStencil.stencilTestEnable = VK_FALSE;
        info->depthStencil.front = {}; // Optional
        info->depthStencil.back = {}; // Optional

        // builderInfo.depthStencil = depthStencil;

        info->pipelineInfo.pDepthStencilState = &info->depthStencil;

        
    //});   
          
    return *this;
}         
          
pipeline::builder& pipeline::builder::buildShader(const char *path, VkShaderStageFlagBits stage)
{         
    addToBuildQueue([this,path,stage](){
        EShLanguage EShStage;
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                EShStage = EShLangVertex;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                EShStage = EShLangFragment;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                EShStage = EShLangGeometry;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                EShStage = EShLangTessControl;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                EShStage = EShLangTessEvaluation;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                EShStage = EShLangCompute;
                break;
            default:
                throw std::runtime_error("invalid shader stage!");
        }
        shader shaderObj(path,EShStage);
        VkShaderModule shaderModule = shaderObj.createShaderModule(inst.device);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.pNext = nullptr;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";

        shaderVectorMutex.lock();
        info->shaderStages.push_back(shaderStageInfo);
        shaderVectorMutex.unlock();

    });   
    
    return *this;
}

pipeline::builder& pipeline::builder::buildVertexInputState(std::vector<VkVertexInputBindingDescription>& descriptions,std::vector<VkVertexInputAttributeDescription>& attributes) {
    //info->descriptions = descriptions;
    //info->attributes = attributes;
    addToBuildQueue([this,&descriptions,&attributes](){
        info->vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        info->vertexInputState.vertexBindingDescriptionCount = descriptions.size();
        info->vertexInputState.pVertexBindingDescriptions = descriptions.data();
        info->vertexInputState.vertexAttributeDescriptionCount = attributes.size();
        info->vertexInputState.pVertexAttributeDescriptions = attributes.data();

        info->pipelineInfo.pVertexInputState = &info->vertexInputState;
    });

    return *this;
}

pipeline::builder& pipeline::builder::buildInputAssembly(VkPrimitiveTopology primitive) {
    addToBuildQueue([this,primitive](){
        info->inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        info->inputAssembly.topology = primitive;
        info->inputAssembly.primitiveRestartEnable = VK_FALSE;

        info->pipelineInfo.pInputAssemblyState = &info->inputAssembly;
    });
    
    return *this;
}

pipeline::builder& pipeline::builder::buildRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace) {
    addToBuildQueue([this,polygonMode,cullMode,frontFace]() {
       info->rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
       info->rasterizer.depthClampEnable = VK_FALSE;
       info->rasterizer.rasterizerDiscardEnable = VK_FALSE;
       info->rasterizer.polygonMode = polygonMode;
       info->rasterizer.lineWidth = 1.0f;
       info->rasterizer.cullMode = cullMode;
       info->rasterizer.frontFace = frontFace;
       info->rasterizer.depthBiasEnable = VK_FALSE;
       info->rasterizer.depthBiasConstantFactor = 0.0f; // Optional
       info->rasterizer.depthBiasClamp = 0.0f; // Optional
       info->rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

       info->pipelineInfo.pRasterizationState = &info->rasterizer;
    });

    return *this;
}

pipeline::builder& pipeline::builder::buildMultisampling() {
    //addToBuildQueue([this]() {
        info->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        info->multisampling.sampleShadingEnable = VK_FALSE;
        info->multisampling.rasterizationSamples = swapChain.samples;
        info->multisampling.minSampleShading = 1.0f; // Optional
        info->multisampling.pSampleMask = nullptr; // Optional
        info->multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        info->multisampling.alphaToOneEnable = VK_FALSE; // Optional

        info->pipelineInfo.pMultisampleState = &info->multisampling;

       

    //});
        
    return *this;
}

pipeline::builder& pipeline::builder::buildColorBlendAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask) {
    addToBuildQueue([this,blendEnable,colorWriteMask]() {
        info->colorBlendAttachment.colorWriteMask = colorWriteMask;

        info->colorBlendAttachment.blendEnable = blendEnable;
        
        if (blendEnable == false) {
            info->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            info->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            info->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            info->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            info->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            info->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
        } else {
            info->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            info->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            info->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            info->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            info->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            info->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
    });

    return *this;
}

pipeline::builder& pipeline::builder::buildColorBlending() { // TODO
    addToBuildQueue([&]() {
        info->colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        info->colorBlending.logicOpEnable = VK_FALSE;
        info->colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        info->colorBlending.attachmentCount = 1;
        info->colorBlending.pAttachments = &info->colorBlendAttachment;
        info->colorBlending.blendConstants[0] = 0.0f; // Optional
        info->colorBlending.blendConstants[1] = 0.0f; // Optional
        info->colorBlending.blendConstants[2] = 0.0f; // Optional
        info->colorBlending.blendConstants[3] = 0.0f; // Optional

        info->pipelineInfo.pColorBlendState = &info->colorBlending;
    });
    
    return *this;
}

pipeline::builder& pipeline::builder::buildViewportState() {
    info->viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info->viewportState.viewportCount = info->viewports.size();
    info->viewportState.scissorCount = info->scissors.size();
    info->viewportState.pViewports = info->viewports.data();
    info->viewportState.pScissors = info->scissors.data();

    info->pipelineInfo.pViewportState = &info->viewportState;
    
    return *this;
}

pipeline::builder& pipeline::builder::buildViewport(float width, float height) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    info->viewports.push_back(viewport);

    return *this;
}

pipeline::builder& pipeline::builder::buildScissor(VkOffset2D offset,VkExtent2D extent) {
    VkRect2D scissor{};
    scissor.offset = offset;
    scissor.extent = extent;
    info->scissors.push_back(scissor);
    
    return *this;
}

pipeline::builder& pipeline::builder::buildDynamicState(std::vector<VkDynamicState> dynamicStates) {
    this->info->dynamicStates = dynamicStates;
    addToBuildQueue([&]() {
        info->dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        info->dynamicStateInfo.dynamicStateCount = this->info->dynamicStates.size();
        info->dynamicStateInfo.pDynamicStates = this->info->dynamicStates.data();

        info->pipelineInfo.pDynamicState = &info->dynamicStateInfo;
    });
    
    return *this;
}

pipeline::builder& pipeline::builder::buildTessellationState(uint32_t patchControlPoints)
{
    addToBuildQueue([this,patchControlPoints](){
        info->tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        info->tessellationState.patchControlPoints = patchControlPoints;

        info->pipelineInfo.pTessellationState = &info->tessellationState;
    });
   
    return *this;
}

pipeline::builder& pipeline::builder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
    info->descriptorSetLayouts.push_back(layout);
    return *this;
}

pipeline pipeline::builder::buildPipeline(VkPipeline oldPipeline) {
    //check that the tasks are completed
    while (pipelineBuildQueue.size() != 0) {
        while (pipelineBuildQueue.front().load() != true) {
            std::this_thread::yield();
        }
        pipelineBuildQueue.pop_front();
    }

    createRenderPass();

    // VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    info->pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info->pipelineLayoutInfo.pSetLayouts = info->descriptorSetLayouts.data();
    info->pipelineLayoutInfo.setLayoutCount = info->descriptorSetLayouts.size();
    
    if (info->pushConstantRange.has_value()) {
        info->pipelineLayoutInfo.pushConstantRangeCount = 1;
        info->pipelineLayoutInfo.pPushConstantRanges = &info->pushConstantRange.value();
    }

    if (vkCreatePipelineLayout(inst.device, &info->pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // VkGraphicsPipelineCreateInfo pipelineInfo{};
    info->pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info->pipelineInfo.stageCount = static_cast<uint32_t>(info->shaderStages.size());
    info->pipelineInfo.pStages = info->shaderStages.data();

    info->pipelineInfo.layout = pipelineLayout;
    info->pipelineInfo.renderPass = renderPass;
    info->pipelineInfo.subpass = 0;

    info->pipelineInfo.basePipelineHandle = oldPipeline; 
    info->pipelineInfo.basePipelineIndex = 0; 

    if (vkCreateGraphicsPipelines(inst.device, VK_NULL_HANDLE, 1, &info->pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    for (auto& shader : info->shaderStages) {
        vkDestroyShaderModule(inst.device, shader.module, nullptr);
    }
    
    return pipeline(inst,swapChain,info,pipelineLayout,renderPass,graphicsPipeline);
}

void pipeline::builder::buildAttachment(VkFormat format,VkSampleCountFlagBits samples, VkImageLayout initialLayout, VkImageLayout finalLayout, VkImageLayout refLayout) {
    //attachmentMutex.lock();
    info->attachments.push_back({});
    info->attachmentRefs.push_back({});
    size_t index = info->attachments.size()-1;
    //attachmentMutex.unlock();
    
    info->attachments[index].format = format;
    info->attachments[index].samples = samples;
    info->attachments[index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    info->attachments[index].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    info->attachments[index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    info->attachments[index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    info->attachments[index].initialLayout = initialLayout;
    info->attachments[index].finalLayout = finalLayout;

    info->attachmentRefs[index].attachment = index;
    info->attachmentRefs[index].layout = refLayout;

}

void pipeline::builder::buildRenderPass() {

    //info->subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //subpass.colorAttachmentCount = 1;
    //subpass.pColorAttachments = &colorAttachmentRef;
    //subpass.pDepthStencilAttachment = &depthAttachmentRef;
    //subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{}; //tells the render pass when to start
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(info->attachments.size());
    renderPassInfo.pAttachments = info->attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &info->subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(inst.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

// builder end

// pipeline


void pipeline::builder::createRenderPass() { //todo abstractions
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.swapChainImageFormat;
    colorAttachment.samples = swapChain.samples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = inst.findDepthFormat();
    depthAttachment.samples = swapChain.samples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChain.swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{}; //tells the render pass when to start
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(inst.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }

}

void pipeline::reCreateSwapChain() {

    int width = 0, height = 0;
    glfwGetFramebufferSize(inst.win.win, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(inst.win.win, &width, &height);
        glfwWaitEvents();
    }

    inst.waitForDeviceIdle();
    
    destroyFramebuffers();
    swapChain.destroyImageViews();
    vkDestroySwapchainKHR(inst.device, swapChain.swapChain, nullptr);

    swapChain.createVKSwapChain();
    swapChain.createImageViews();
    createDepthResources();
    createColorResources();
    createFramebuffers();

}

// framebuffers

void pipeline::createDepthResources()
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = swapChain.swapChainExtent.width;
    imageCreateInfo.extent.height = swapChain.swapChainExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = inst.findDepthFormat();
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.samples = swapChain.samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    depthImage = inst.createImage(imageCreateInfo, allocCreateInfo);
    inst.createImageView(depthImage, imageCreateInfo.format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

    inst.transitionImageLayout(depthImage, imageCreateInfo.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void pipeline::createColorResources() {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = swapChain.swapChainExtent.width;
    imageCreateInfo.extent.height = swapChain.swapChainExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = swapChain.swapChainImageFormat; 
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageCreateInfo.samples = swapChain.samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    colorImage = inst.createImage(imageCreateInfo, allocCreateInfo);
    inst.createImageView(colorImage, imageCreateInfo.format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
}

void pipeline::createFramebuffers()
{
    frameBuffers.resize(swapChain.swapChainImageViews.size());

    for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++) {
        std::array<VkImageView,3> attachments = {
            colorImage.view.value(),
            depthImage.view.value(),
            swapChain.swapChainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChain.swapChainExtent.width;
        framebufferInfo.height = swapChain.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(inst.device, &framebufferInfo, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void pipeline::destroyFramebuffers() {
    for (auto &framebuffer : frameBuffers) {
        vkDestroyFramebuffer(inst.device, framebuffer, nullptr);
    }
    inst.destroyImage(depthImage);
    inst.destroyImage(colorImage);
}

// framebuffers end

// pipeline end

} // namespace graphics

namespace compute {

pipeline::pipeline(instance& inst,BuildInfo* buildInfo,VkPipelineLayout pipelineLayout,VkPipeline computePipeline) 
    : inst(inst),builderInfo(buildInfo),pipelineLayout(pipelineLayout),computePipeline(computePipeline)
{
}

pipeline::~pipeline() {
    vkDestroyPipelineLayout(inst.device, pipelineLayout, nullptr);
    vkDestroyPipeline(inst.device, computePipeline, nullptr);
}

pipeline::builder pipeline::builder::begin(instance& inst) {
    return builder(inst);
}

pipeline::builder::builder(instance& inst)
    : inst(inst),info(new BuildInfo{}),threadPool(threadpool::get_instance()) 
{
    
}
void pipeline::builder::addToBuildQueue(std::function<void()> func)
{
    pipelineBuildQueue.emplace_back(false);
    threadPool->add_task(func,&pipelineBuildQueue.back());
}

pipeline::builder& pipeline::builder::buildShader(const char* path, VkShaderStageFlagBits stage) {
    addToBuildQueue([this,path,stage](){
        EShLanguage EShStage;
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                EShStage = EShLangVertex;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                EShStage = EShLangFragment;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                EShStage = EShLangGeometry;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                EShStage = EShLangTessControl;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                EShStage = EShLangTessEvaluation;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                EShStage = EShLangCompute;
                break;
            default:
                throw std::runtime_error("invalid shader stage!");
        }
        shader shaderObj(path,EShStage);
        VkShaderModule shaderModule = shaderObj.createShaderModule(inst.device);

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.pNext = nullptr;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";

        info->shaderStage = shaderStageInfo;

    });  

    return *this;
}

pipeline::builder& pipeline::builder::addDescriptorSetLayout(VkDescriptorSetLayout layout) {
    info->descriptorSetLayouts.push_back(layout);
    return *this;
}

pipeline::builder& pipeline::builder::buildPipelineLayout() {
    addToBuildQueue([this](){
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = info->descriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = info->descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(inst.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }
        
    });
    return *this;
}

pipeline pipeline::builder::buildPipeline(VkPipeline oldPipeline) {
    while (pipelineBuildQueue.size() != 0) {
        while (pipelineBuildQueue.front().load() != true) {
            std::this_thread::yield();
        }
        pipelineBuildQueue.pop_front();
    }

    info->pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info->pipelineInfo.layout = pipelineLayout;
    info->pipelineInfo.stage = info->shaderStage;

    info->pipelineInfo.basePipelineIndex = 0;
    info->pipelineInfo.basePipelineHandle = oldPipeline;

    if (vkCreateComputePipelines(inst.device, VK_NULL_HANDLE, 1, &info->pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    return pipeline(inst,info,pipelineLayout,computePipeline);
}



} // namespace compute


// if something goes wrong I have the old stuff
// void pipeline::createGraphicsPipeline(const char* vert,const char* frag) {
//     shader vertShader(vert, EShLangVertex);
//     shader fragShader(frag, EShLangFragment);

//     VkShaderModule vertShaderModule = vertShader.createShaderModule(inst.device);
//     VkShaderModule fragShaderModule = fragShader.createShaderModule(inst.device);

//     VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//     vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//     vertShaderStageInfo.module = vertShaderModule;
//     vertShaderStageInfo.pName = "main";

//     VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//     fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//     fragShaderStageInfo.module = fragShaderModule;
//     fragShaderStageInfo.pName = "main";

//     VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

//     //--------------------------------------------------------------------------------//
//     std::vector<VkDynamicState> dynamicStates = {
//         VK_DYNAMIC_STATE_VIEWPORT,
//         VK_DYNAMIC_STATE_SCISSOR
//     };

//     VkPipelineDynamicStateCreateInfo dynamicState{};
//     dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
//     dynamicState.pDynamicStates = dynamicStates.data();
    
//     VkPipelineViewportStateCreateInfo viewportState{};
//     viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     viewportState.viewportCount = 1;
//     viewportState.scissorCount = 1;
//     //--------------------------------------------------------------------------------//


//     VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//     vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     vertexInputInfo.vertexBindingDescriptionCount = 0;
//     vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
//     vertexInputInfo.vertexAttributeDescriptionCount = 0;
//     vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

//     VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//     inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     inputAssembly.primitiveRestartEnable = VK_FALSE;

//     VkPipelineRasterizationStateCreateInfo rasterizer{};
//     rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     rasterizer.depthClampEnable = VK_FALSE;
//     rasterizer.rasterizerDiscardEnable = VK_FALSE;
//     rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//     rasterizer.lineWidth = 1.0f;
//     rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//     rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//     rasterizer.depthBiasEnable = VK_FALSE;
//     rasterizer.depthBiasConstantFactor = 0.0f; // Optional
//     rasterizer.depthBiasClamp = 0.0f; // Optional
//     rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

//     VkPipelineMultisampleStateCreateInfo multisampling{};
//     multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     multisampling.sampleShadingEnable = VK_FALSE;
//     multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//     multisampling.minSampleShading = 1.0f; // Optional
//     multisampling.pSampleMask = nullptr; // Optional
//     multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//     multisampling.alphaToOneEnable = VK_FALSE; // Optional

//     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//     colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     colorBlendAttachment.blendEnable = VK_FALSE;
//     colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//     colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//     colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//     colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//     colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//     colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

//     // colorBlendAttachment.blendEnable = VK_TRUE;
//     // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//     // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//     // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//     // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//     // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//     // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

//     VkPipelineColorBlendStateCreateInfo colorBlending{};
//     colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     colorBlending.logicOpEnable = VK_FALSE;
//     colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
//     colorBlending.attachmentCount = 1;
//     colorBlending.pAttachments = &colorBlendAttachment;
//     colorBlending.blendConstants[0] = 0.0f; // Optional
//     colorBlending.blendConstants[1] = 0.0f; // Optional
//     colorBlending.blendConstants[2] = 0.0f; // Optional
//     colorBlending.blendConstants[3] = 0.0f; // Optional

//     VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//     pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//     pipelineLayoutInfo.setLayoutCount = 0; // Optional
//     pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
//     pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//     pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

//     if (vkCreatePipelineLayout(inst.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
//         throw std::runtime_error("failed to create pipeline layout!");
//     }

//     VkGraphicsPipelineCreateInfo pipelineInfo{};
//     pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     pipelineInfo.stageCount = 2;
//     pipelineInfo.pStages = shaderStages;
//     pipelineInfo.pVertexInputState = &vertexInputInfo;
//     pipelineInfo.pInputAssemblyState = &inputAssembly;
//     pipelineInfo.pViewportState = &viewportState;
//     pipelineInfo.pRasterizationState = &rasterizer;
//     pipelineInfo.pMultisampleState = &multisampling;
//     pipelineInfo.pDepthStencilState = nullptr; // Optional
//     pipelineInfo.pColorBlendState = &colorBlending;
//     pipelineInfo.pDynamicState = &dynamicState;

//     pipelineInfo.layout = pipelineLayout;
//     pipelineInfo.renderPass = renderPass;
//     pipelineInfo.subpass = 0;

//     pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
//     pipelineInfo.basePipelineIndex = -1; // Optional

//     if (vkCreateGraphicsPipelines(inst.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
//         throw std::runtime_error("failed to create graphics pipeline!");
//     }


//     vkDestroyShaderModule(inst.device, vertShaderModule, nullptr);
//     vkDestroyShaderModule(inst.device, fragShaderModule, nullptr);
// }


} // namespace svklib

#endif // SVKLIB_PIPELINE_CPP










