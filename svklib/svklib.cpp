#ifndef SVKLIB_CPP
#define SVKLIB_CPP

#include "svklib.hpp"

namespace svklib {

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType,VkImageAspectFlags aspectFlags,uint32_t mipLevels) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void createBuffer(VmaAllocator vmaAllocator,void* data,VkDeviceSize size) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer buffer;
    VmaAllocation alloc;
    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(vmaAllocator,&createInfo,&allocCreateInfo,&buffer,&alloc,&allocInfo);

    //could use vmaMapMemory() but instead we used VMA_ALLOCATION_VREATE_MAPPED_BIT

    memcpy(allocInfo.pMappedData,data,size);

    

    vmaDestroyBuffer(vmaAllocator,buffer,alloc);
    

}

/*@mFilter - VK_FILTER_NEAREST/VK_FILTER_LINEAR
 *@samplerAddressMode - VK_SAMPLER_ADDRESS_MODE_REPEAT/VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT/VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE/VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE/VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
*/
VkSampler createSampler2D(VkDevice device,VkPhysicalDevice physicalDevice,VkFilter mFilter,VkSamplerAddressMode samplerAddressMode) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = mFilter; //or VK_FILTER_NEAREST
    samplerInfo.minFilter = mFilter; 

    // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the image.
    samplerInfo.addressModeU = samplerAddressMode;
    samplerInfo.addressModeV = samplerAddressMode;
    samplerInfo.addressModeW = samplerAddressMode;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties); //query maxSamplerAnisotropy

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    // beyond image clamp
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    // field specifies which coordinate system you want to use to address texels in an image
    // VK_FALSE = 0,1
    // VK_TRUE = 0,texWidth 0,texHeight
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE; //IDK MAN I THINK THIS IS RIGHT

    VkSampler sampler;
    if (vkCreateSampler(device, &samplerInfo, NULL, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    
    return sampler;
}

} // namespace svklib

#endif // SVKLIB_CPP