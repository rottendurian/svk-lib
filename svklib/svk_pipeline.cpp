#ifndef SVKLIB_PIPELINE_CPP
#define SVKLIB_PIPELINE_CPP

#include "svk_pipeline.hpp"

namespace svklib {

namespace graphics {

pipeline::pipeline(instance& inst,swapchain& swapChain) 
    : inst(inst),swapChain(swapChain),
      threadPool(svklib::threadpool::get_instance())
{
    addToBuildQueue([this](){
        createDepthResources();
        createRenderPass();
        createFramebuffers();
    });
}

pipeline::~pipeline() {
    
    destroyFramebuffers();
    vkDestroyPipeline(inst.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(inst.device, pipelineLayout, nullptr);
    vkDestroyRenderPass(inst.device, renderPass, nullptr);
}

// void pipeline::freeDescriptorSets()
// {
//     for (int i = 0; i < descriptorSets.size(); i++) {
//         vkFreeDescriptorSets(inst.device, descriptorPool, descriptorSets[i].size(), descriptorSets[i].data());
//     }
// }

// builder

void pipeline::addToBuildQueue(std::function<void()> func)
{
    pipelineBuildQueue.emplace_back(false);
    threadPool->add_task(func,&pipelineBuildQueue.back());
}

void pipeline::buildDepthStencil()
{
    addToBuildQueue([this](){
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        builderInfo.depthStencil = depthStencil;
    });

}

void pipeline::buildShader(const char *path, VkShaderStageFlagBits stage)
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
        builderInfo.shaderStages.push_back(shaderStageInfo);
        shaderVectorMutex.unlock();
    });
    
}

// void pipeline::buildShader(std::initializer_list<const char*> paths, VkShaderStageFlagBits stage) {
//     EShLanguage EShStage;
//     switch (stage) {
//         case VK_SHADER_STAGE_VERTEX_BIT:
//             EShStage = EShLangVertex;
//             break;
//         case VK_SHADER_STAGE_FRAGMENT_BIT:
//             EShStage = EShLangFragment;
//             break;
//         case VK_SHADER_STAGE_GEOMETRY_BIT:
//             EShStage = EShLangGeometry;
//             break;
//         case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
//             EShStage = EShLangTessControl;
//             break;
//         case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
//             EShStage = EShLangTessEvaluation;
//             break;
//         case VK_SHADER_STAGE_COMPUTE_BIT:
//             EShStage = EShLangCompute;
//             break;
//         default:
//             throw std::runtime_error("invalid shader stage!");
//     }
//     shader shaderObj(paths,EShStage);
//     VkShaderModule shaderModule = shaderObj.createShaderModule(inst.device);

//     VkPipelineShaderStageCreateInfo shaderStageInfo{};
//     shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     shaderStageInfo.pNext = nullptr;
//     shaderStageInfo.stage = stage;
//     shaderStageInfo.module = shaderModule;
//     shaderStageInfo.pName = "main";

//     builderInfo.shaderStages.push_back(shaderStageInfo);
// }

void pipeline::buildVertexInputState(std::vector<VkVertexInputBindingDescription> &descriptions,std::vector<VkVertexInputAttributeDescription> &attributes) {
    addToBuildQueue([this,&descriptions,&attributes](){
        builderInfo.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        builderInfo.vertexInputState.vertexBindingDescriptionCount = descriptions.size();
        builderInfo.vertexInputState.pVertexBindingDescriptions = descriptions.data();
        builderInfo.vertexInputState.vertexAttributeDescriptionCount = attributes.size();
        builderInfo.vertexInputState.pVertexAttributeDescriptions = attributes.data();
    });
}

void pipeline::buildInputAssembly(VkPrimitiveTopology primitive) {
    addToBuildQueue([this,primitive](){
        builderInfo.inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        builderInfo.inputAssembly.topology = primitive;
        builderInfo.inputAssembly.primitiveRestartEnable = VK_FALSE;
    });
}

void pipeline::buildRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace) {
    addToBuildQueue([this,polygonMode,cullMode,frontFace]() {
        builderInfo.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        builderInfo.rasterizer.depthClampEnable = VK_FALSE;
        builderInfo.rasterizer.rasterizerDiscardEnable = VK_FALSE;
        builderInfo.rasterizer.polygonMode = polygonMode;
        builderInfo.rasterizer.lineWidth = 1.0f;
        builderInfo.rasterizer.cullMode = cullMode;
        builderInfo.rasterizer.frontFace = frontFace;
        builderInfo.rasterizer.depthBiasEnable = VK_FALSE;
        builderInfo.rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        builderInfo.rasterizer.depthBiasClamp = 0.0f; // Optional
        builderInfo.rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    });
}

void pipeline::buildMultisampling(VkSampleCountFlagBits sampleCount) {
    addToBuildQueue([this,sampleCount]() {
        builderInfo.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        builderInfo.multisampling.sampleShadingEnable = VK_FALSE;
        builderInfo.multisampling.rasterizationSamples = sampleCount;
        builderInfo.multisampling.minSampleShading = 1.0f; // Optional
        builderInfo.multisampling.pSampleMask = nullptr; // Optional
        builderInfo.multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        builderInfo.multisampling.alphaToOneEnable = VK_FALSE; // Optional
    });
        
}

void pipeline::buildColorBlendAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask) {
    addToBuildQueue([this,blendEnable,colorWriteMask]() {
        builderInfo.colorBlendAttachment.colorWriteMask = colorWriteMask;

        builderInfo.colorBlendAttachment.blendEnable = blendEnable;
        
        if (blendEnable == false) {
            builderInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            builderInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            builderInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            builderInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            builderInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            builderInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
        } else {
            builderInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            builderInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            builderInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            builderInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            builderInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            builderInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
    });

}

void pipeline::buildColorBlending() { // todo
    addToBuildQueue([this]() {
        builderInfo.colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        builderInfo.colorBlending.logicOpEnable = VK_FALSE;
        builderInfo.colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        builderInfo.colorBlending.attachmentCount = 1;
        builderInfo.colorBlending.pAttachments = &builderInfo.colorBlendAttachment;
        builderInfo.colorBlending.blendConstants[0] = 0.0f; // Optional
        builderInfo.colorBlending.blendConstants[1] = 0.0f; // Optional
        builderInfo.colorBlending.blendConstants[2] = 0.0f; // Optional
        builderInfo.colorBlending.blendConstants[3] = 0.0f; // Optional
    });
}

void pipeline::buildViewportState() {
    builderInfo.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    builderInfo.viewportState.viewportCount = builderInfo.viewports.size();
    builderInfo.viewportState.scissorCount = builderInfo.scissors.size();
    builderInfo.viewportState.pViewports = builderInfo.viewports.data();
    builderInfo.viewportState.pScissors = builderInfo.scissors.data();
}

void pipeline::buildViewport(float width, float height) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    builderInfo.viewports.push_back(viewport);
}

void pipeline::buildScissor(VkOffset2D offset,VkExtent2D extent) {
    VkRect2D scissor{};
    scissor.offset = offset;
    scissor.extent = extent;
    builderInfo.scissors.push_back(scissor);
}

void pipeline::buildDynamicState(std::vector<VkDynamicState> &&dynamicStates) {
    builderInfo.dynamicStates = dynamicStates;
    addToBuildQueue([this]() {
        builderInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        builderInfo.dynamicStateInfo.dynamicStateCount = builderInfo.dynamicStates.size();
        builderInfo.dynamicStateInfo.pDynamicStates = builderInfo.dynamicStates.data();
    });
}

void pipeline::buildTessellationState(uint32_t patchControlPoints)
{
    addToBuildQueue([this,patchControlPoints](){
        builderInfo.tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        builderInfo.tessellationState.patchControlPoints = patchControlPoints;
    });
}

void pipeline::buildPipeline() {

    //check that the tasks are completed
    while (pipelineBuildQueue.size() != 0) {
        while (pipelineBuildQueue.front().load() != true) {
            std::this_thread::yield();
        }
        pipelineBuildQueue.pop_front();
    }
    // for (int i = 0; i < pipelineBuildQueue.size(); i++) {
    //     while(pipelineBuildQueue[i].load() != true) {
    //         std::this_thread::yield();
    //     }
    // }
    // pipelineBuildQueue.clear();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantCount;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; 

    if (vkCreatePipelineLayout(inst.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(builderInfo.shaderStages.size());
    pipelineInfo.pStages = builderInfo.shaderStages.data();
    pipelineInfo.pVertexInputState = &builderInfo.vertexInputState;
    pipelineInfo.pInputAssemblyState = &builderInfo.inputAssembly;
    pipelineInfo.pViewportState = &builderInfo.viewportState;
    pipelineInfo.pRasterizationState = &builderInfo.rasterizer;
    pipelineInfo.pMultisampleState = &builderInfo.multisampling;
    pipelineInfo.pDepthStencilState = &builderInfo.depthStencil;
    pipelineInfo.pColorBlendState = &builderInfo.colorBlending;
    pipelineInfo.pDynamicState = &builderInfo.dynamicStateInfo;
    pipelineInfo.pTessellationState = &builderInfo.tessellationState;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(inst.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    for (auto& shader : builderInfo.shaderStages) {
        vkDestroyShaderModule(inst.device, shader.module, nullptr);
    }
    
}

// builder end

// pipeline

void pipeline::createGraphicsPipeline() {
    // auto start = std::chrono::steady_clock::now();
    // buildShader(inputInfo.vert, VK_SHADER_STAGE_VERTEX_BIT);
    // buildShader(inputInfo.frag, VK_SHADER_STAGE_FRAGMENT_BIT);
    // auto end = std::chrono::steady_clock::now();
    // std::cout << "shader build time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

    // buildVertexInputState(std::move(inputInfo.vertexInputBindingDescriptions), std::move(inputInfo.vertexInputAttributeDescriptions));
    // builderInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    // builderInfo.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    // buildDynamicState();

    // buildInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // buildViewport(swapChain.swapChainExtent.width, swapChain.swapChainExtent.height);
    // buildScissor({0,0}, {swapChain.swapChainExtent.width,swapChain.swapChainExtent.height});
    // buildViewportState();

    // buildRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // buildMultisampling();
    // buildDepthStencil();     
    // buildColorBlendAttachment(false);
    // buildColorBlending();

    // buildPipeline();
    
}

void pipeline::createRenderPass() { //todo abstractions
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChain.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = inst.findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{}; //tells the render pass when to start
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

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
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    depthImage = inst.createImage(imageCreateInfo, allocCreateInfo);
    inst.createImageView(depthImage, imageCreateInfo.format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

    inst.transitionImageLayout(depthImage, imageCreateInfo.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void pipeline::createFramebuffers()
{
    frameBuffers.resize(swapChain.swapChainImageViews.size());

    for (size_t i = 0; i < swapChain.swapChainImageViews.size(); i++) {
        std::array<VkImageView,2> attachments = {
            swapChain.swapChainImageViews[i],
            depthImage.view.value()
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
}

// framebuffers end

// pipeline end

} // namespace graphics

namespace compute {

pipeline::pipeline(instance& inst) 
    : inst(inst)
{
    this->threadPool = svklib::threadpool::get_instance();
}

pipeline::~pipeline() {
    vkDestroyPipelineLayout(inst.device, pipelineLayout, nullptr);
    vkDestroyPipeline(inst.device, computePipeline, nullptr);
}

void pipeline::addToBuildQueue(std::function<void()> func)
{
    pipelineBuildQueue.emplace_back(false);
    threadPool->add_task(func,&pipelineBuildQueue.back());
}

void pipeline::buildShader(const char* path, VkShaderStageFlagBits stage) {
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

        builderInfo.shaderStage = shaderStageInfo;

    });   
}

void pipeline::buildPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    builderInfo.descriptorSetLayouts = descriptorSetLayouts;
    addToBuildQueue([this](){
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = builderInfo.descriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = builderInfo.descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(inst.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }
    });
}

void pipeline::buildPipeline() {
    while (pipelineBuildQueue.size() != 0) {
        while (pipelineBuildQueue.front().load() != true) {
            std::this_thread::yield();
        }
        pipelineBuildQueue.pop_front();
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage = builderInfo.shaderStage;

    if (vkCreateComputePipelines(inst.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }
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










