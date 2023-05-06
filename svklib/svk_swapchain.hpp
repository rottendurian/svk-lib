#ifndef SVKLIB_SWAPCHAIN_HPP
#define SVKLIB_SWAPCHAIN_HPP

// #include "pch.hpp"
#include "svk_instance.hpp"

#include <cstdint>
#include <algorithm> 

namespace svklib {

class swapchain {
friend class graphics::pipeline;
friend class renderer;
public:
    swapchain(instance& inst, int framesInFlight, VkImageUsageFlagBits imageFlags,VkPresentModeKHR presentMode=VK_PRESENT_MODE_MAILBOX_KHR);
    swapchain(instance& inst, int framesInFlight, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_2_BIT, VkPresentModeKHR preferredPresentMode=VK_PRESENT_MODE_MAILBOX_KHR);
    ~swapchain();
    
private:
    //references
    instance& inst;
    //references end
    //swapchain
public:
    const VkSampleCountFlagBits samples;
    const int framesInFlight;
    VkCommandBuffer* commandBuffers;
    int currentFrame = 0;

    VkSwapchainKHR swapChain{};
    std::vector<VkImage> swapChainImages{};
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
    void recreateSwapChain();
private:
    VkPresentModeKHR preferredPresentMode;
    VkImageUsageFlags imageFlags;
    instance::CommandPool commandPool;
    void allocateCommandBuffers();
    void freeCommandBuffers();

    void createVKSwapChain();
    // void reCreateSwapChain(VkRenderPass renderPass);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void destroySwapChain();
    //swapchain end

    //swapchain views
public:
    std::vector<VkImageView> swapChainImageViews{};
private:
    void createImageViews();
    void destroyImageViews();

    //swapchain views end



};




} // namespace svklib


#endif // SVKLIB_SWAPCHAIN_HPP
