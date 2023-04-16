#ifndef SVKLIB_SWAPCHAIN_HPP
#define SVKLIB_SWAPCHAIN_HPP

#include "pch.hpp"
#include "svk_instance.hpp"

#include <cstdint>
#include <algorithm> 

namespace svklib {

class swapchain {
friend class graphics::pipeline;
friend class renderer;
public:
    swapchain(instance& inst, int framesInFlight);
    swapchain(instance& inst, int framesInFlight, VkPresentModeKHR preferredPresentMode);
    ~swapchain();
    
private:
    //references
    instance& inst;
    //references end
    //swapchain
public:
    const int framesInFlight;
    VkCommandBuffer* commandBuffers;
    int currentFrame = 0;

    VkSwapchainKHR swapChain{};
    std::vector<VkImage> swapChainImages{};
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
private:
    VkPresentModeKHR preferredPresentMode;
    instance::CommandPool commandPool;
    void allocateCommandBuffers();
    void freeCommandBuffers();

    void createVKSwapChain();
    // void reCreateSwapChain(VkRenderPass renderPass);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    //swapchain end

    //swapchain views
    std::vector<VkImageView> swapChainImageViews{};

    void createImageViews();
    void destroyImageViews();

    //swapchain views end

    void destroySwapChain();


};




} // namespace svklib


#endif // SVKLIB_SWAPCHAIN_HPP