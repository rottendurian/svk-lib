#ifndef SVKLIB_PIPELINE_HPP
#define SVKLIB_PIPELINE_HPP

#include "svk_swapchain.hpp"
#include "svk_shader.hpp"
#include "svk_threadpool.hpp"
#include <vulkan/vulkan_core.h>

namespace svklib {

namespace graphics {
    class pipeline {
    public:
        class builder;
    private:
        friend class svklib::renderer;
        friend class svklib::graphics::pipeline::builder;
        
        struct BuildInfo {
            std::vector<VkDynamicState> dynamicStates{};
            VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

            std::vector<VkVertexInputBindingDescription> descriptions;
            std::vector<VkVertexInputAttributeDescription> attributes;
            VkPipelineVertexInputStateCreateInfo vertexInputState{};

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            VkPipelineViewportStateCreateInfo viewportState{};
            std::vector<VkViewport> viewports{};
            std::vector<VkRect2D> scissors{};
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            VkPipelineMultisampleStateCreateInfo multisampling{};
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            VkPipelineColorBlendStateCreateInfo colorBlending{}; //might not need
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            VkPipelineTessellationStateCreateInfo tessellationState{};

            std::optional<VkPushConstantRange> pushConstantRange{};

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts; 
            
            //abstraction for subpasses
            std::vector<VkAttachmentDescription> attachments;
            std::vector<VkAttachmentReference> attachmentRefs;
            VkSubpassDescription subpass{};

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            VkGraphicsPipelineCreateInfo pipelineInfo{};
        };

        pipeline(instance& inst,swapchain& swapchain, BuildInfo* builderInfo, 
                 VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipeline graphicsPipeline);

    public:
        //for forward declarations
        pipeline(instance& inst, swapchain& swapchain);
        ~pipeline();


    private:
        instance& inst;
        swapchain& swapChain;

        //pipeline
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;
        VkPipeline graphicsPipeline;
        //pipeline end

        void reCreateSwapChain();
        
        //framebuffers
        instance::svkimage depthImage;
        void createDepthResources();
        instance::svkimage colorImage;
        void createColorResources();
        std::vector<VkFramebuffer> frameBuffers{};
        void createFramebuffers();
        void destroyFramebuffers();
        //framebuffers end

        
        std::unique_ptr<BuildInfo> builderInfo;
    public:
        //doesnt need to be in the pipeline
        std::vector<VkBuffer> vertexBuffers;
        std::vector<VkDeviceSize> vertexBufferOffsets;
        IndexBufferInfo indexBufferInfo;
        std::vector<std::vector<VkDescriptorSet>> descriptorSets;
        
        void updatePushConstantData(void* data);

        std::optional<VkPushConstantRange> pushConstantRange;
        void* pushConstantData = nullptr;

        class builder {
            public:
                static builder begin(instance& inst, swapchain& swapChain);
                builder& buildShader(const char* path, VkShaderStageFlagBits stage);
                builder& buildVertexInputState(std::vector<VkVertexInputBindingDescription> descriptions,std::vector<VkVertexInputAttributeDescription> attributes);
                builder& buildInputAssembly(VkPrimitiveTopology primitive);
                builder& buildRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
                builder& buildDepthStencil();
                builder& buildMultisampling();
                builder& buildColorBlendAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask=VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT); 
                builder& buildColorBlending();
                builder& buildViewport(float width, float height);
                builder& buildScissor(VkOffset2D offset,VkExtent2D extent);
                builder& buildViewportState();
                builder& buildDynamicState(std::vector<VkDynamicState> dynamicStates);
                builder& buildTessellationState(uint32_t patchControlPoints);

                builder& buildPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
                builder& addDescriptorSetLayout(VkDescriptorSetLayout layout);
               
                void buildPipeline(VkPipeline oldPipeline, pipeline* pipeline);
                pipeline buildPipeline(VkPipeline oldPipeline);

            private:
                void buildPipelineImpl(VkPipeline oldPipeline);


                builder(instance& inst, swapchain& swapChain);
                //~builder();
               
                instance& inst;
                swapchain& swapChain;
                svklib::threadpool* threadPool;
                
                void addToBuildQueue(std::function<void()> func);
                std::deque<std::atomic_bool> pipelineBuildQueue;
                std::mutex shaderVectorMutex;

                BuildInfo* info;
                
                std::mutex attachmentMutex;
                void createRenderPass();
                void buildAttachment(VkFormat format,VkSampleCountFlagBits samples, VkImageLayout initialLayout, VkImageLayout finalLayout, VkImageLayout refLayout);
                void buildRenderPass();

                VkPipelineLayout pipelineLayout;
                VkRenderPass renderPass;
                VkPipeline graphicsPipeline;
        };


    };

} // namespace graphics

namespace compute {

    class pipeline {
        public:
            class builder;
        private:
            friend class builder;
            friend class svklib::renderer;
            struct BuildInfo {
                VkPipelineShaderStageCreateInfo shaderStage{};
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts{};
                VkPushConstantRange pushConstantRange{};
                VkComputePipelineCreateInfo pipelineInfo{};
            };

            pipeline(instance& inst, BuildInfo* buildInfo, VkPipelineLayout pipelineLayout, VkPipeline pipeline);
        public:
            //for forward declarations
            pipeline(instance& inst);
            ~pipeline();

            std::unique_ptr<BuildInfo> builderInfo{};

            VkPipelineLayout pipelineLayout;
            VkPipeline computePipeline;

            std::vector<std::vector<VkDescriptorSet>> descriptorSets;

        private:
            instance& inst;
            
        public:
            class builder {
            public:
                ~builder()=default;
                
                static builder begin(instance& inst);
                builder& buildShader(const char* path, VkShaderStageFlagBits stage);
                builder& addDescriptorSetLayout(VkDescriptorSetLayout layout);
                builder& buildPushConstant(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
                builder& buildPipelineLayout();
                void buildPipeline(VkPipeline oldPipeline, svklib::compute::pipeline* pipeline);
                svklib::compute::pipeline buildPipeline(VkPipeline oldPipeline);
                
            private:
                void buildPipelineImpl(VkPipeline oldPipeline);

                builder(instance& inst);
                instance& inst;

                BuildInfo* info;
                svklib::threadpool* threadPool;
                std::deque<std::atomic_bool> pipelineBuildQueue;
                void addToBuildQueue(std::function<void()> func);

                VkPipelineLayout pipelineLayout;
                VkPipeline computePipeline;

            };





    };

} // namespace compute

} // namespace svklib


#endif // SVKLIB_PIPELINE_CPP
