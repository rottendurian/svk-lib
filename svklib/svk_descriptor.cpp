#ifndef SVKLIB_DESCRIPTOR_CPP
#define SVKLIB_DESCRIPTOR_CPP

#include "svk_descriptor.hpp"

namespace svklib {

namespace descriptor {

//allocator class start

static constexpr struct PoolSizes {
	std::array<std::pair<VkDescriptorType, float>, 11> sizes = {{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
	}};
} s_PoolSizes;

allocator* allocator::init(VkDevice device)
{
	allocator* m_allocator = new allocator();
    m_allocator->device = device;
	return m_allocator;
}

void allocator::cleanup() {
	// s_allocator->~allocator();
	// delete s_allocator;
}

VkDevice allocator::getDevice()
{
    return device;
}

allocator_pool allocator::getPool()
{
	if (device == VK_NULL_HANDLE) {
		throw std::runtime_error("allocator class was not initialized (device is VK_NULL_HANDLE)");
	}
    return allocator_pool(this, borrowPool());
}

allocator::allocator() {}

VkDescriptorPool allocator::createDescriptorPool(int count, VkDescriptorPoolCreateFlags flags)
{
	VkDescriptorPoolSize* sizes = (VkDescriptorPoolSize*)alloca(s_PoolSizes.sizes.size()*sizeof(VkDescriptorPoolSize));

	for (int i = 0; i < s_PoolSizes.sizes.size(); i++) {
		sizes[i] = {s_PoolSizes.sizes[i].first, static_cast<uint32_t>(s_PoolSizes.sizes[i].second*count)};
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = flags;
	pool_info.maxSets = count;
	pool_info.poolSizeCount = static_cast<uint32_t>(s_PoolSizes.sizes.size());
	pool_info.pPoolSizes = sizes;

	VkDescriptorPool descriptorPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

	return descriptorPool;
}

allocator::~allocator()
{
	while (availablePools.size() > 0) {
		vkDestroyDescriptorPool(device,availablePools.front(),nullptr);
		availablePools.pop_front();
	}
}

VkDescriptorPool allocator::borrowPool()
{
	VkDescriptorPool currentPool;

	if (availablePools.size() == 0) {
		currentPool = createDescriptorPool(1000,VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT); //TODO maybe set to framesinflight? idk
	} else {
		std::lock_guard<std::mutex> lock(poolMutex);
		currentPool = availablePools.front();
		availablePools.pop_front();
	}

	return currentPool;
}

void allocator::returnPool(allocator_pool& pool) {
	vkResetDescriptorPool(device,pool.currentPool,0); //TODO might be worth it to just pop these off since I am using a deque instead of this optimization
	for (auto& d_pool : pool.usedPools) {
		vkResetDescriptorPool(device,d_pool,0);
	}
	std::lock_guard<std::mutex> lock(poolMutex);
	
	availablePools.push_back(pool.currentPool);
	pool.currentPool = VK_NULL_HANDLE;
	for (auto& d_pool : pool.usedPools) {
		availablePools.push_back(d_pool);
	}
	pool.usedPools.clear();
}

//DescriptorPool struct start

VkDescriptorSet allocator_pool::allocate(VkDescriptorSetLayout layout) {
	VkDescriptorSet set;
	if (currentPool == VK_NULL_HANDLE){
		currentPool = allocator->borrowPool();
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;

	allocInfo.pSetLayouts = &layout;
	allocInfo.descriptorPool = currentPool;
	allocInfo.descriptorSetCount = 1;

	//try to allocate the descriptor set
	VkResult allocResult = vkAllocateDescriptorSets(allocator->device, &allocInfo, &set);
	bool needReallocate = false;

	switch (allocResult) {
	case VK_SUCCESS:
		//all good, return
		return set;
	case VK_ERROR_FRAGMENTED_POOL:
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		//reallocate pool
		needReallocate = true;
		break;
	default:
		//unrecoverable error

		throw std::runtime_error("Failed to allocate descriptor set (unrecoverable error)");
	}

	if (needReallocate){
		//allocate a new pool and retry
		usedPools.push_back(currentPool);
		currentPool = allocator->borrowPool();

		allocResult = vkAllocateDescriptorSets(allocator->device, &allocInfo, &set);

		//if it still fails then we have big issues
		if (allocResult == VK_SUCCESS){
			return set;
		}
	}

	throw std::runtime_error("Failed to allocate descriptor set (after realloc)");
}

allocator_pool::allocator_pool(svklib::descriptor::allocator *allocator, VkDescriptorPool pool)
	: allocator(allocator), currentPool(pool)
{}

allocator_pool::~allocator_pool() {
	returnPool();
}

void allocator_pool::returnPool()
{
	allocator->returnPool(*this);
}

//DescriptorPool struct end

//allocator class end



//layoutcache class start

void layout_cache::init(VkDevice newDevice){
	device = newDevice;
}

void layout_cache::cleanup(){
	//delete every descriptor layout held
	for (auto pair : layoutCache){
		vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
	}
}

VkDescriptorSetLayout layout_cache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo* info) {
	DescriptorLayoutInfo layoutinfo;
	layoutinfo.bindings.reserve(info->bindingCount);
	bool isSorted = true;
	int lastBinding = -1;

	//copy from the direct info struct into our own one
	for (int i = 0; i < info->bindingCount; i++) {
		layoutinfo.bindings.push_back(info->pBindings[i]);

		//check that the bindings are in strict increasing order
		if (info->pBindings[i].binding > lastBinding) {
			lastBinding = info->pBindings[i].binding;
		}
		else {
			isSorted = false;
		}
	}
	//sort the bindings if they aren't in order
	if (!isSorted) {
		std::sort(layoutinfo.bindings.begin(), layoutinfo.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b ){
				return a.binding < b.binding;
		});
	}

	//try to grab from cache
    layoutCacheMutex.lock();
	auto it = layoutCache.find(layoutinfo);
	if (it != layoutCache.end()){
        layoutCacheMutex.unlock();
		return (*it).second;
	}
	else {
		//create a new one (not found)
		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

		//add to cache
		layoutCache[layoutinfo] = layout;
        layoutCacheMutex.unlock();
		return layout;
    }
}

bool layout_cache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const{
	if (other.bindings.size() != bindings.size()){
		return false;
	} else {
		//compare each of the bindings is the same. Bindings are sorted so they will match
		for (int i = 0; i < bindings.size(); i++) {
			if (other.bindings[i].binding != bindings[i].binding){
				return false;
			}
			if (other.bindings[i].descriptorType != bindings[i].descriptorType){
				return false;
			}
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount){
				return false;
			}
			if (other.bindings[i].stageFlags != bindings[i].stageFlags){
				return false;
			}
		}
		return true;
	}
}

size_t layout_cache::DescriptorLayoutInfo::hash() const {
    using std::size_t;
    using std::hash;

    size_t result = hash<size_t>()(bindings.size());

    for (const VkDescriptorSetLayoutBinding& b : bindings)
    {
        //pack the binding data into a single int64. Not fully correct but it's ok
        size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

        //shuffle the packed binding data and xor it with the main hash
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}

//layoutcache class end



//builder class start

builder builder::begin(layout_cache* layoutCache, allocator_pool* pool){

	builder builder;

	builder.cache = layoutCache;
	builder.alloc = pool;
	return builder;
}

void builder::bind_buffer(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags) {
    //create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding newBinding{};

    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;

    bindings.push_back(newBinding);

    //create the descriptor write
    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;

    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pBufferInfo = nullptr;
    newWrite.dstBinding = binding;

    writes.push_back(newWrite);
}

void builder::bind_image(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags) {
    //create the descriptor binding for the layout
    VkDescriptorSetLayoutBinding newBinding{};

    newBinding.descriptorCount = 1;
    newBinding.descriptorType = type;
    newBinding.pImmutableSamplers = nullptr;
    newBinding.stageFlags = stageFlags;
    newBinding.binding = binding;

    bindings.push_back(newBinding);

    //create the descriptor write
    VkWriteDescriptorSet newWrite{};
    newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    newWrite.pNext = nullptr;

    newWrite.descriptorCount = 1;
    newWrite.descriptorType = type;
    newWrite.pImageInfo = nullptr;
    newWrite.dstBinding = binding;

    writes.push_back(newWrite);
}

void builder::update_buffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {
    writes[binding].pBufferInfo = bufferInfo;
}

void builder::update_image(uint32_t binding, VkDescriptorImageInfo *imageInfo) {
    writes[binding].pImageInfo = imageInfo;
}

VkDescriptorSetLayout builder::buildLayout()
{
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;

	layoutInfo.pBindings = bindings.data();
	layoutInfo.bindingCount = bindings.size();

	layout = cache->create_descriptor_layout(&layoutInfo);

	return layout;
}

VkDescriptorSet builder::buildSet() {

	//allocate descriptor
	VkDescriptorSet set = alloc->allocate(layout);

	//write descriptor
	for (VkWriteDescriptorSet& w : writes) {
		w.dstSet = set;
	}

	vkUpdateDescriptorSets(alloc->allocator->getDevice(), writes.size(), writes.data(), 0, nullptr);

	return set;
}

//builder class end

} // namespace descriptor

} // namespace svklib

#endif // SVKLIB_DESCRIPTOR_CPP
