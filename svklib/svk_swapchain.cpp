#ifndef SVKLIB_SWAPCHAIN_CPP
#define SVKLIB_SWAPCHAIN_CPP

#include "svk_swapchain.hpp"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

namespace svklib {

swapchain::swapchain(instance& inst,int framesInFlight,VkSampleCountFlagBits samples) 
    : inst(inst),samples(samples),framesInFlight(framesInFlight),commandPool(inst.getCommandPool()),
      preferredPresentMode(VK_PRESENT_MODE_MAILBOX_KHR) 
{
    if (samples > inst.maxMsaa) {
        throw std::runtime_error("swapchain samples cannot be higher than physical device capabilities");
    }
    createVKSwapChain();
    createImageViews();
    allocateCommandBuffers();
}

swapchain::swapchain(instance& inst,int framesInFlight,VkSampleCountFlagBits samples,VkPresentModeKHR presentMode) 
    : inst(inst),framesInFlight(framesInFlight),samples(samples),commandPool(inst.getCommandPool()),
      preferredPresentMode(presentMode)
{
    createVKSwapChain();
    createImageViews();
    allocateCommandBuffers();
}

swapchain::~swapchain() {
    freeCommandBuffers();
    commandPool.returnPool();
    destroySwapChain();
}

// swapchain
void swapchain::createVKSwapChain() {
    instance::SwapChainSupportDetails swapChainSupport = inst.querySwapChainSupport(inst.physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = inst.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    instance::QueueFamilyIndices indices = inst.findQueueFamilies(inst.physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    if (vkCreateSwapchainKHR(inst.device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(inst.device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(inst.device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void swapchain::destroySwapChain() {
    destroyImageViews();
    vkDestroySwapchainKHR(inst.device, swapChain, nullptr);
}

// void swapchain::reCreateSwapChain(VkRenderPass rendererPass) {

//     int width = 0, height = 0;
//     glfwGetFramebufferSize(inst.win.win, &width, &height);
//     while (width == 0 || height == 0) {
//         glfwGetFramebufferSize(inst.win.win, &width, &height);
//         glfwWaitEvents();
//     }

//     vkDeviceWaitIdle(inst.device);
    
//     destroyFramebuffers();
//     destroyImageViews();
//     vkDestroySwapchainKHR(inst.device, swapChain, nullptr);

//     createSwapChain();
//     createImageViews();
//     createFramebuffers(rendererPass);
// }

VkSurfaceFormatKHR swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

void swapchain::allocateCommandBuffers() {
    commandBuffers = new VkCommandBuffer[framesInFlight];
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool.get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(framesInFlight);
 
    if (vkAllocateCommandBuffers(inst.device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void swapchain::freeCommandBuffers()
{
    vkFreeCommandBuffers(inst.device, commandPool.get(), static_cast<uint32_t>(framesInFlight), commandBuffers);
    delete[] commandBuffers;
}

VkPresentModeKHR swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == preferredPresentMode) {
            return availablePresentMode;
        }
        // if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        //     return availablePresentMode;
        // }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(inst.win.win, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
//swapchain end

//swapchain views

void swapchain::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(inst.device, swapChainImages[i], swapChainImageFormat,VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);   
    }
}

void swapchain::destroyImageViews() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(inst.device, imageView, nullptr);
    }
}

// swapchain views end


} // namespace svklib

#endif // SVKLIB_SWAPCHAIN_CPP
