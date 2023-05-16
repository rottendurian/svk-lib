#ifndef SVKLIB_PCH_HPP
#define SVKLIB_PCH_HPP

#include <cstdlib>
#include <cstring>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <chrono>
#include <memory>
#include <cstdint>
#include <algorithm> 
#include <unordered_map>
#include <map>
#include <optional>
#include <set>
#include <memory>

#include <deque>
#include <functional>

#include <thread>
#include <mutex>
#include <atomic>

// #define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace svklib {

extern VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType,VkImageAspectFlags aspectFlags,uint32_t mipLevels);

} // namespace svklib

#endif // SVKLIB_PCH_HPP
