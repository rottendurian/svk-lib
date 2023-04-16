#ifndef SVKLIB_HPP
#define SVKLIB_HPP

#include "svk_window.hpp"
#include "svk_instance.hpp"
#include "svk_swapchain.hpp"
#include "svk_pipeline.hpp"
#include "svk_renderer.hpp"
#include "svk_threadpool.hpp"

namespace svklib {

extern VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType,VkImageAspectFlags aspectFlags,uint32_t mipLevels);
extern VkSampler createSampler2D(VkDevice device,VkPhysicalDevice physicalDevice,VkFilter mFilter,VkSamplerAddressMode samplerAddressMode);

} // namespace svklib

#endif // SVKLIB_HPP