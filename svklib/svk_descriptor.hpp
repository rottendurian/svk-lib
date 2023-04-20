#ifndef SVKLIB_DESCRIPTOR_HPP
#define SVKLIB_DESCRIPTOR_HPP

// #include "pch.hpp"
#include <unordered_map>

namespace svklib {

namespace descriptor {

class allocator {
    friend class pool;
public:
    struct pool {
        svklib::descriptor::allocator* allocator{nullptr};
        std::deque<VkDescriptorPool> usedPools;
        VkDescriptorPool currentPool{VK_NULL_HANDLE};

        VkDescriptorSet allocate(VkDescriptorSetLayout layout);
        pool(svklib::descriptor::allocator* allocator, VkDescriptorPool pool);
        pool(const pool&) = delete;
        void operator=(pool const&) = delete;
        ~pool();
        void returnPool();
    };
    static allocator* init(VkDevice device);
    void cleanup();
    VkDevice getDevice();
    pool getPool();
    ~allocator();
private:
    allocator();
    allocator(const allocator&) = delete;
    void operator=(allocator const&) = delete;
    
    VkDevice device{VK_NULL_HANDLE};

    VkDescriptorPool createDescriptorPool(int count, VkDescriptorPoolCreateFlags flags);

    std::mutex poolMutex;
    std::deque<VkDescriptorPool> availablePools;

    VkDescriptorPool borrowPool();
    void returnPool(pool& pool);
};

namespace layout {

class cache {
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

} // namespace layout

class builder {
public:
	static builder begin(layout::cache* layoutCache, allocator::pool* pool);

	void bind_buffer(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags);
	void bind_image(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags);

    void update_buffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    void update_image(uint32_t binding, VkDescriptorImageInfo* imageInfo);

    VkDescriptorSetLayout buildLayout();

	VkDescriptorSet buildSet();
private:

	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayout layout;

	layout::cache* cache;
	allocator::pool* alloc;
};

} // namespace descriptor

} // namespace svklib

#endif // SVKLIB_DESCRIPTOR_HPP


