#ifndef SVKLIB_PIPELINE_HPP
#define SVKLIB_PIPELINE_HPP

// #include "pch.hpp"
#include "svk_swapchain.hpp"
#include "svk_shader.hpp"
#include "svk_threadpool.hpp"



namespace svklib {

namespace graphics {

    class pipeline {
        friend class renderer;
    public:
        pipeline(instance& inst,swapchain& swapchain);
        ~pipeline();

        //doesnt need to be in the pipeline
        std::vector<VkBuffer> vertexBuffers;
        std::vector<VkDeviceSize> vertexBufferOffsets;
        IndexBufferInfo indexBufferInfo;
        std::vector<std::vector<VkDescriptorSet>> descriptorSets;
        void* pushConstantData = nullptr;

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        VkPushConstantRange pushConstantRange;


    private:
        // void freeDescriptorSets();
        // need to borrow a descriptorPool

        //references
        instance& inst;
        swapchain& swapChain;
        svklib::threadpool* threadPool;
        //references end
        struct BuilderInfo {
            std::vector<VkDynamicState> dynamicStates{};
            VkPipelineDynamicStateCreateInfo dynamicStateInfo{};

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
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
            
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        };
        VkGraphicsPipelineCreateInfo pipelineInfo{};
    public:
        //builder
        BuilderInfo builderInfo{};
        
        void addToBuildQueue(std::function<void()> func);

        void buildDepthStencil();
        void buildShader(const char* path, VkShaderStageFlagBits stage);
        // void buildShader(std::initializer_list<const char*> paths, VkShaderStageFlagBits stage); //this functions shader initializer doesn't work
        void buildVertexInputState(std::vector<VkVertexInputBindingDescription> &descriptions,std::vector<VkVertexInputAttributeDescription> &attributes);
        void buildInputAssembly(VkPrimitiveTopology primitive);
        void buildRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
        void buildMultisampling(VkSampleCountFlagBits sampleCount=VK_SAMPLE_COUNT_1_BIT);
        // below may need more functionality in the future 
        void buildColorBlendAttachment(VkBool32 blendEnable, VkColorComponentFlags colorWriteMask=VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT); 
        // I genuinely don't know what this does
        void buildColorBlending();
        void buildViewport(float width, float height);
        void buildScissor(VkOffset2D offset,VkExtent2D extent);
        void buildViewportState();
        void buildDynamicState(std::vector<VkDynamicState> &&dynamicStates);
        void buildTessellationState(uint32_t patchControlPoints);

        void buildPipeline();
        
    private:
        std::deque<std::atomic_bool> pipelineBuildQueue;
        std::mutex shaderVectorMutex;
        //builder end
    private:

        //pipeline
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;
        VkPipeline graphicsPipeline;
        
        void createGraphicsPipeline();
        void createRenderPass(); //todo abstractions
        //pipeline end
    public:
    private:
        void reCreateSwapChain();

        //framebuffers
        instance::svkimage depthImage;
        void createDepthResources();
        std::vector<VkFramebuffer> frameBuffers{};
        void createFramebuffers();
        void destroyFramebuffers();
        //framebuffers end
    };

} // namespace graphics

namespace compute {

    class pipeline {
        friend class renderer;
        public:
            pipeline(instance& inst);
            ~pipeline();

            struct BuildInfo {
                VkPipelineShaderStageCreateInfo shaderStage;
                std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            };
            BuildInfo builderInfo{};

            void addToBuildQueue(std::function<void()> func);
            void buildShader(const char* path, VkShaderStageFlagBits stage);
            void buildPipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
            void buildPipeline();

        private:
            std::deque<std::atomic_bool> pipelineBuildQueue;

            VkPipelineLayout pipelineLayout;
            VkPipeline computePipeline;

            std::vector<VkDescriptorSet> descriptorSets;
            VkComputePipelineCreateInfo pipelineInfo{};

        private:
            //References
            instance& inst;
            svklib::threadpool* threadPool;
            //References end



    };

} // namespace compute

} // namespace svklib


#endif // SVKLIB_PIPELINE_CPP