#ifndef SVKLIB_INSTANCE_HPP
#define SVKLIB_INSTANCE_HPP

#include "svk_window.hpp"
#include "svk_descriptor.hpp"
#include "svk_threadpool.hpp"
#include <vulkan/vulkan_core.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <optional>
#include <set>
#include <memory>

namespace svklib {

namespace graphics {
    class pipeline;
    struct IndexBufferInfo {
        VkBuffer buff;
        VkDeviceSize offset;
        uint32_t count;
        VkIndexType type;
    };
}
namespace compute {
    class pipeline;
}
class swapchain;
class renderer;

class instance {
    friend class swapchain;
    friend class graphics::pipeline;
    friend class renderer;
public:
    instance(window& win,uint32_t apiVersion);
    instance(window& win,uint32_t apiVersion,VkPhysicalDeviceFeatures enabledFeatures);
    instance(window& win,uint32_t apiVersion,VkPhysicalDeviceFeatures enabledFeatures,std::vector<const char*> extensions);

    void init();
    ~instance();

    inline void waitForDeviceIdle() { vkDeviceWaitIdle(device); }
    
    const uint32_t apiVersion;

private:
    //references
    window& win;
    //references end
#ifdef _DEBUG
    static constexpr bool enableValidationLayers = true;
#else
    static constexpr bool enableValidationLayers = false;
#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    std::optional<QueueFamilyIndices> queueFamilies;
public:
    //VKINSTANCE
    VkInstance inst;
private:
    void createInstance(const char *engineName,const char* appName,unsigned int engineVersion,unsigned int appVersion,unsigned int apiVersion);
    void destroyInstance();

    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    //VKINSTANCE END

    //DEBUGGER
    VkDebugUtilsMessengerEXT debugger;

    void createDebugMessenger();
    void destroyDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    
    VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
    //DEBUGGER END

    //VKPHYSICALDEVICE
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface{};

    void createSurface();
    void pickPhysicalDevice();
    int ratePhysicalDevice(VkPhysicalDevice physicalDevice);
    bool isDeviceSuitable(VkPhysicalDevice physicalDevice);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);

    //for depth buffer
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,VkImageTiling tiling, VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);
    VkFormat findDepthFormat();
    
    VkSampleCountFlagBits maxMsaa = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits getMaxUsableSampleCount();
    //VKPHYSICALDEVICE END

public:
    inline VkSampleCountFlagBits getMaxMsaa() {return maxMsaa;}
    //VKDEVICE
    VkDevice device;

    //sychronized queue class
    class svkqueue {
    private:
        friend class instance;
        VkQueue queue;
        std::mutex mutex;
        svkqueue();
        svkqueue(VkQueue queue);
        svkqueue(const svkqueue&) = delete;
        svkqueue& operator=(const svkqueue&) = delete;
    public:
        VkResult submit(uint32_t submitCount,const VkSubmitInfo* submitInfo, VkFence fence);
        VkResult submit(const VkSubmitInfo& submitInfo, VkFence fence);
        VkResult present(const VkPresentInfoKHR* presentInfo);
        VkResult waitIdle();
    };

    svkqueue graphicsQueue;
    svkqueue presentQueue;

private:
    class CommandPool {
    private:
        friend class instance;
        CommandPool(instance& inst, const VkCommandPool commandPool, const int id);
    public:
        ~CommandPool();
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;
        VkCommandPool get();
        void returnPool();
    private:
        instance& inst;
        VkCommandPool pool;
        const int id;
    };
    
    std::vector<VkCommandPool> commandPools;
    std::vector<bool> commandPoolBorrowed;
    std::mutex commandPoolMutex;

    void createCommandPools(int count);
    void destroyCommandPools();
    CommandPool borrowCommandPool();
    void returnCommandPool(int id);

public:
    // If a commandpool is used to make command buffers that are reused,
    // this command pool should not be returned until the command buffers are no longer in use
    CommandPool getCommandPool();


private:
    VkCommandPool createCommandPool();
    void destroyCommandPool(VkCommandPool commandPool);

    std::unique_ptr<VkPhysicalDeviceFeatures> requestedFeatures;
    std::vector<const char*> requestedExtensions;

    int getSupportedFeatureScore(VkPhysicalDevice physicalDevice); 
    void createLogicalDevice();
    void destroyLogicalDevice();
    //VDEVICE END
public:
    //Vulkan Memory Allocator
    VmaAllocator allocator;
private:
    void createAllocator();
    void destroyAllocator();

public:
    struct svkbuffer {
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        VkBuffer buff;
        VkDeviceSize size;
        VkDescriptorBufferInfo getBufferInfo();
        VkDescriptorBufferInfo getBufferInfo(VkDeviceSize offset);
        graphics::IndexBufferInfo getIndexBufferInfo(VkIndexType indexType);
        graphics::IndexBufferInfo getIndexBufferInfo(VkIndexType indexType, VkDeviceSize offset);
    };
    svkbuffer createBuffer(VkBufferCreateInfo bufferCreateInfo,VmaAllocationCreateInfo allocCreateInfo, VkDeviceSize size);
    svkbuffer createStagingBuffer(VkDeviceSize size);
    //Transfer bit already selected
    svkbuffer createBufferStaged(VkBufferUsageFlags bufferUsage, VkDeviceSize size,const void* data,VkCommandPool commandPool);
    //Transfer bit already selected
    svkbuffer createBufferStaged(VkBufferUsageFlags bufferUsage, VkDeviceSize size,const void* data);
    svkbuffer createUniformBuffer(VkDeviceSize size);
    void copyBufferToBuffer(VkCommandPool commandPool,VkBuffer src, VkBuffer dst,VkDeviceSize size);
    struct svkimage {
        VmaAllocation alloc;
        VmaAllocationInfo allocInfo;
        VkImage image;
        std::optional<VkImageView> view;
        std::optional<VkSampler> sampler;
        uint32_t mipLevels;
        VkDescriptorImageInfo getImageInfo();
    };
    svkimage createImage(VkImageCreateInfo imageInfo, VmaAllocationCreateInfo allocInfo);
    void transitionImageLayout(svkimage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,VkCommandPool commandPool);
    void transitionImageLayout(svkimage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(svkbuffer& buffer, svkimage& image, VkExtent3D extent,VkCommandPool commandPool);
    void copyBufferToImage(svkbuffer& buffer, svkimage& image, VkExtent3D extent);
    
    svkimage createImageStaged(VkImageCreateInfo imageInfo, VmaAllocationCreateInfo allocInfo, VkCommandPool commandPool, const void* data);
    svkimage create2DImageFromFile(const char* file,uint32_t mipLevels,VkCommandPool commandPool);
    svkimage create2DImageFromFile(const char* file,uint32_t mipLevels);

    void copyImageToImage(svkimage& src, svkimage& dst, VkExtent3D extent,VkCommandPool commandPool); //UNTESTED
    void copyImageToImage(svkimage& src, svkimage& dst, VkExtent3D extent); //UNTESTED

    void createImageView(svkimage& image, VkFormat format, VkImageViewType viewType,VkImageAspectFlags aspectFlags);
    void createSampler(svkimage& image, VkFilter mFilter,VkSamplerAddressMode samplerAddressMode);
    void generateMipmaps(svkimage& image, VkFormat imageFormat, VkOffset3D texSize,VkCommandPool commandPool);

    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
    void endSingleTimeCommands(VkCommandPool commandPool,VkCommandBuffer commandBuffer);
    void destroyBuffer(svkbuffer& buffer);
    void destroyImage(svkimage& image);
    void destroyImageView(VkImageView imageView);
    void destroySampler(VkSampler sampler);
    
private:
    //Vulkan Memory Allocator end

    //Descriptor Allocators
public:
    descriptor::allocator::pool getDescriptorPool();
    descriptor::builder createDescriptorBuilder(descriptor::allocator::pool* pool);
private:
    descriptor::allocator* descriptorAllocator;
    descriptor::layout::cache descriptorLayoutCache;
    //Descriptor Allocators end

};

} // namespace svklib

#endif // SVKLIB_INSTANCE_HPP
