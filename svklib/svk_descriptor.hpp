#ifndef SVKLIB_DESCRIPTOR_HPP
#define SVKLIB_DESCRIPTOR_HPP

#include "pch.hpp"

namespace svklib {

class DescriptorAllocator {
    friend class DescriptorPool;
public:
    struct DescriptorPool {
        DescriptorAllocator* allocator{nullptr};
        std::deque<VkDescriptorPool> usedPools;
        VkDescriptorPool currentPool{VK_NULL_HANDLE};

        VkDescriptorSet allocate(VkDescriptorSetLayout layout);
        DescriptorPool(const DescriptorPool&) = delete;
        void operator=(DescriptorPool const&) = delete;
        ~DescriptorPool();
        void returnPool();
    };
    static DescriptorAllocator* init(VkDevice device);
    void cleanup();
    VkDevice getDevice();
    DescriptorPool getPool();
    ~DescriptorAllocator();
private:
    DescriptorAllocator();
    DescriptorAllocator(const DescriptorAllocator&) = delete;
    void operator=(DescriptorAllocator const&) = delete;
    
    VkDevice device{VK_NULL_HANDLE};

    VkDescriptorPool createDescriptorPool(int count, VkDescriptorPoolCreateFlags flags);

    std::mutex poolMutex;
    std::deque<VkDescriptorPool> availablePools;

    VkDescriptorPool borrowPool();
    void returnPool(DescriptorPool& pool);
};

class DescriptorLayoutCache {
public:
    void init(VkDevice newDevice);
    void cleanup();

    //unordered map is sychronized
    VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info);

    struct DescriptorLayoutInfo {
        //good idea to turn this into an inlined array
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorLayoutInfo& other) const;

        size_t hash() const;
    };

private:
    struct DescriptorLayoutHash {
        std::size_t operator()(const DescriptorLayoutInfo& k) const{
            return k.hash();
        }
    };

    std::mutex layoutCacheMutex;
    std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
    VkDevice device;
};

class DescriptorBuilder {
public:
	static DescriptorBuilder begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator::DescriptorPool* pool);

	DescriptorBuilder& bind_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
	DescriptorBuilder& bind_image(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
    void update_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    void update_image(uint32_t binding, VkDescriptorImageInfo* imageInfo);

	bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
	bool build(VkDescriptorSet& set);
private:

	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	DescriptorLayoutCache* cache;
	DescriptorAllocator::DescriptorPool* alloc;
};

} // namespace svklib


#endif // SVKLIB_DESCRIPTOR_HPP


