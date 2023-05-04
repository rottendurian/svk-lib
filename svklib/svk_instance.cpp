#ifndef SVKLIB_INSTANCE_CPP
#define SVKLIB_INSTANCE_CPP

#include "svk_instance.hpp"

#include "svk_shader.hpp"
#include "vulkan/vulkan_core.h"
#include <cstring>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <glslang/Public/ShaderLang.h>

namespace svklib {

static const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

static constexpr int validationLayersSize = 1;
static inline const char* const validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

instance::instance(window &win,uint32_t apiVersion)
    : win(win),apiVersion(apiVersion),requestedFeatures(nullptr)
{
    init();
}

instance::instance(window &win,uint32_t apiVersion,VkPhysicalDeviceFeatures enabledFeatures)
    :win(win),apiVersion(apiVersion),requestedFeatures(std::make_unique<VkPhysicalDeviceFeatures>(enabledFeatures))
{
    init();
}

instance::instance(window &win,uint32_t apiVersion,VkPhysicalDeviceFeatures enabledFeatures,std::vector<const char*> extensions)
    :win(win),apiVersion(apiVersion),requestedFeatures(std::make_unique<VkPhysicalDeviceFeatures>(enabledFeatures)),requestedExtensions(extensions)
{
    init();
}

static bool contains_string(const std::vector<const char*>& strings, const char* target)
{
    if (strings.empty())
        return false;

    auto it = std::find_if(strings.begin(), strings.end(),
                           [target](const char* str) { return std::strcmp(str, target) == 0; });
    return it != strings.end();
}

void instance::init() {
    if (!contains_string(requestedExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        requestedExtensions.push_back(swapchainExtension);

    createInstance("svklib","svklib",0,1,apiVersion);
    createDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
    createCommandPools(std::thread::hardware_concurrency());
    descriptorAllocator = descriptor::allocator::init(device);
    descriptorLayoutCache.init(device);
    glslang::InitializeProcess();

}

instance::~instance() {
    glslang::FinalizeProcess();
    descriptorLayoutCache.cleanup();
    delete descriptorAllocator;
    destroyCommandPools();
    destroyAllocator();
    destroyLogicalDevice();
    vkDestroySurfaceKHR(inst, surface, NULL);
    destroyDebugMessenger();
    destroyInstance();
}

//VKINSTANCE

void instance::createInstance(const char *engineName,const char* appName,unsigned int engineVersion,unsigned int appVersion,unsigned int apiVersion) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but not supported");
    }
    
    VkApplicationInfo appInfo; memset(&appInfo,0,sizeof(appInfo));
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = engineName;
    appInfo.engineVersion = engineVersion;
    appInfo.apiVersion = apiVersion;

    shader::setShaderVersion(apiVersion);

    VkInstanceCreateInfo createInfo; memset(&createInfo,0,sizeof(createInfo));
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // uint32_t extensionCount = 0;
    // const char** extensions = getRequiredExtensions(&extensionCount);

    std::vector<const char*> extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = (uint32_t)validationLayersSize;
        createInfo.ppEnabledLayerNames = validationLayers;

        debugCreateInfo = populateDebugMessengerCreateInfo();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    // VkInstance instance;
    if (vkCreateInstance(&createInfo, NULL, &inst) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");        
    }

}

void instance::destroyInstance() {
    vkDestroyInstance(inst,NULL);
}

std::vector<const char*> instance::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool instance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties* availableLayers = (VkLayerProperties*)malloc(layerCount*sizeof(VkLayerProperties));
    
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (int i = 0; i < validationLayersSize; i++) {
        bool layerFound = false;
        for (int k = 0; k < layerCount; k++) {
            if (strcmp(validationLayers[i], availableLayers[k].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            free(availableLayers);
            return false;
        }
    }

    free(availableLayers);
    return true;

}

//VKINSTANCE END

//DEBUG MESSENGER
static FILE* loggingFile = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL instance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    fprintf(loggingFile,"validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT instance::populateDebugMessengerCreateInfo() {
    if (loggingFile == nullptr) {
        loggingFile = fopen("validation.log","w");
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = NULL; // Optional
    return createInfo;
}

static inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

void instance::createDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = populateDebugMessengerCreateInfo();

    if (CreateDebugUtilsMessengerEXT(inst, &createInfo, NULL, &debugger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void instance::destroyDebugMessenger() {
    if (!enableValidationLayers) return;
    DestroyDebugUtilsMessengerEXT(inst,debugger,NULL);
}
//DEBUG MESSENGER END

//VKPHYSICALDEVICE
void instance::createSurface() {
    surface = win.createSurface(inst);
}

void instance::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(inst, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(inst, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> deviceRatings;
    //rate physicalDevice
    for (const auto& device : devices) {
        int score = ratePhysicalDevice(device);
        deviceRatings.insert(std::make_pair(score, device));
    }

    for (auto device = deviceRatings.rbegin(); device != deviceRatings.rend(); device++) {
        VkPhysicalDevice d = device->second;
        if (isDeviceSuitable(d)) {
            physicalDevice = d;
            maxMsaa = getMaxUsableSampleCount();
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

int instance::ratePhysicalDevice(VkPhysicalDevice physicalDevice) {
    int score = getSupportedFeatureScore(physicalDevice);
    if (score == 0) return 0;
   
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    VkDeviceSize deviceMemory = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        const auto& heap = memProps.memoryHeaps[i];
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            deviceMemory += heap.size;
        }
    }

    static constexpr double memoryScoreMultiplier = 20.0;
    score += static_cast<int>((static_cast<double>(deviceMemory) / 1000000000.0 * memoryScoreMultiplier) + 0.5);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    switch (properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score+=1200;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score+=600;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score+=300;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score+=10;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            score+=1;
            break;
        default:
            score = 0;
            return score;
            break;
    }

#ifdef _DEBUG 
    std::cout << "Device: " + std::string(properties.deviceName) + " score: " + std::to_string(score) << std::endl;
    std::cout << "Device memory score " << (static_cast<double>(deviceMemory) / 1000000000.0 * memoryScoreMultiplier) << std::endl;
#endif
    return score;
}

bool instance::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

instance::QueueFamilyIndices instance::findQueueFamilies(VkPhysicalDevice physicalDevice) {
    if (queueFamilies.has_value() == true) {
        return queueFamilies.value();
    }
    QueueFamilyIndices indices{};
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamiliesProperties.data());

    int i = 0;
    for (const auto& queueFamily : queueFamiliesProperties) {

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT ) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    if (indices.isComplete()) {
        queueFamilies = indices;
        return indices;
    } else {
        throw std::runtime_error("failed to find queue families!");
    }


    // return indices;
}

bool instance::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions{};

    for (auto& ext : requestedExtensions) {
        requiredExtensions.insert(ext);
    }

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

instance::SwapChainSupportDetails instance::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkFormat instance::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkImageUsageFlags usage)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            if ((props.linearTilingFeatures & usage) == usage) {
                return format;
            }
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            if ((props.optimalTilingFeatures & usage) == usage) {
                return format;
            }

        }
    }

    throw std::runtime_error("failed to find supported format!");
}

VkFormat instance::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        0
    );
}

bool instance::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkSampleCountFlagBits instance::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

// VKPHYSICALDEVICE END

//VKDEVICE

//SVKQUEUE CLASS
instance::svkqueue::svkqueue()
    : queue(VK_NULL_HANDLE), mutex()
{}

instance::svkqueue::svkqueue(VkQueue queue)
    : queue(queue), mutex()
{}

VkResult instance::svkqueue::submit(uint32_t submitCount, const VkSubmitInfo *submitInfo, VkFence fence)
{
    mutex.lock();
    VkResult result = vkQueueSubmit(queue,submitCount,submitInfo,fence);
    mutex.unlock();
    return result;
}

VkResult instance::svkqueue::submit(const VkSubmitInfo &submitInfo, VkFence fence)
{
    mutex.lock();
    VkResult result = vkQueueSubmit(queue,1,&submitInfo,fence);
    mutex.unlock();
    return result;
}

VkResult instance::svkqueue::present(const VkPresentInfoKHR *presentInfo)
{
    mutex.lock();
    VkResult result = vkQueuePresentKHR(queue,presentInfo);
    mutex.unlock();
    return result;
}

VkResult instance::svkqueue::waitIdle()
{
    mutex.lock();
    VkResult result = vkQueueWaitIdle(queue);
    mutex.unlock();
    return result;
}

//SVKQUEUE CLASS END

void instance::createCommandPools(int count)
{
    commandPools.resize(count);
    commandPoolBorrowed.resize(count);
    for (int i = 0; i < count; i++) {
        commandPools[i] = createCommandPool();
        commandPoolBorrowed[i] = false;
    }
}

void instance::destroyCommandPools()
{
    for (int i = 0; i < commandPools.size(); i++) {
        destroyCommandPool(commandPools[i]);
    }
}

instance::CommandPool instance::borrowCommandPool()
{
    std::lock_guard<std::mutex> lock(commandPoolMutex);

    for (int i = 0; i < commandPoolBorrowed.size(); i++) {
        if (!commandPoolBorrowed[i]) {
            commandPoolBorrowed[i] = true;
            return CommandPool(*this,commandPools[i],i);
        }
    }

    commandPools.emplace_back(createCommandPool());
    commandPoolBorrowed.emplace_back(true);
    return CommandPool(*this,commandPools.back(),commandPools.size()-1);
}

void instance::returnCommandPool(int id)
{
    std::lock_guard<std::mutex> lock(commandPoolMutex);
    commandPoolBorrowed[id] = false;
    vkResetCommandPool(device,commandPools[id],0);
}

instance::CommandPool instance::getCommandPool()
{
    return borrowCommandPool();
}

VkCommandPool instance::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    return commandPool;
}

void instance::destroyCommandPool(VkCommandPool commandPool)
{
    vkDestroyCommandPool(device, commandPool, nullptr);
}

#define VKDEVICE_ERROR_CHECK(feature) \
        std::string std_error_msg = "Device feature " + std::string(feature) + " is not supported by this device"; \
        throw std::runtime_error(std_error_msg); \

#define CHECK_VKDEVICE_FEATURE(feature) { \
    if (supportedFeatures.feature == false && requestedFeatures->feature == true) { \
        return 0; \
    } else { \
        if (requestedFeatures->feature == true) \
            featuresScore += 100; \
        else { \
            featuresScore += 10; \
        } \
    } \
} \

int instance::getSupportedFeatureScore(VkPhysicalDevice physicalDevice) {
    int featuresScore = 0;

    if (requestedFeatures == nullptr) 
        return featuresScore;

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    CHECK_VKDEVICE_FEATURE(robustBufferAccess);
    CHECK_VKDEVICE_FEATURE(fullDrawIndexUint32);
    CHECK_VKDEVICE_FEATURE(imageCubeArray);
    CHECK_VKDEVICE_FEATURE(independentBlend);
    CHECK_VKDEVICE_FEATURE(geometryShader);
    CHECK_VKDEVICE_FEATURE(tessellationShader);
    CHECK_VKDEVICE_FEATURE(sampleRateShading);
    CHECK_VKDEVICE_FEATURE(dualSrcBlend);
    CHECK_VKDEVICE_FEATURE(logicOp);
    CHECK_VKDEVICE_FEATURE(multiDrawIndirect);
    CHECK_VKDEVICE_FEATURE(drawIndirectFirstInstance);
    CHECK_VKDEVICE_FEATURE(depthClamp);
    CHECK_VKDEVICE_FEATURE(depthBiasClamp);
    CHECK_VKDEVICE_FEATURE(fillModeNonSolid);
    CHECK_VKDEVICE_FEATURE(depthBounds);
    CHECK_VKDEVICE_FEATURE(wideLines);
    CHECK_VKDEVICE_FEATURE(largePoints);
    CHECK_VKDEVICE_FEATURE(alphaToOne);
    CHECK_VKDEVICE_FEATURE(multiViewport);
    CHECK_VKDEVICE_FEATURE(samplerAnisotropy);
    CHECK_VKDEVICE_FEATURE(textureCompressionETC2);
    CHECK_VKDEVICE_FEATURE(textureCompressionASTC_LDR);
    CHECK_VKDEVICE_FEATURE(textureCompressionBC);
    CHECK_VKDEVICE_FEATURE(occlusionQueryPrecise);
    CHECK_VKDEVICE_FEATURE(pipelineStatisticsQuery);
    CHECK_VKDEVICE_FEATURE(vertexPipelineStoresAndAtomics);
    CHECK_VKDEVICE_FEATURE(fragmentStoresAndAtomics);
    CHECK_VKDEVICE_FEATURE(shaderTessellationAndGeometryPointSize);
    CHECK_VKDEVICE_FEATURE(shaderImageGatherExtended);
    CHECK_VKDEVICE_FEATURE(shaderStorageImageExtendedFormats);
    CHECK_VKDEVICE_FEATURE(shaderStorageImageMultisample);
    CHECK_VKDEVICE_FEATURE(shaderStorageImageReadWithoutFormat);
    CHECK_VKDEVICE_FEATURE(shaderStorageImageWriteWithoutFormat);
    CHECK_VKDEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing);
    CHECK_VKDEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing);
    CHECK_VKDEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing);
    CHECK_VKDEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing);
    CHECK_VKDEVICE_FEATURE(shaderClipDistance);
    CHECK_VKDEVICE_FEATURE(shaderCullDistance);
    CHECK_VKDEVICE_FEATURE(shaderFloat64);
    CHECK_VKDEVICE_FEATURE(shaderInt64);
    CHECK_VKDEVICE_FEATURE(shaderInt16);
    CHECK_VKDEVICE_FEATURE(shaderResourceResidency);
    CHECK_VKDEVICE_FEATURE(shaderResourceMinLod);
    CHECK_VKDEVICE_FEATURE(sparseBinding);
    CHECK_VKDEVICE_FEATURE(sparseResidencyBuffer);
    CHECK_VKDEVICE_FEATURE(sparseResidencyImage2D);
    CHECK_VKDEVICE_FEATURE(sparseResidencyImage3D);
    CHECK_VKDEVICE_FEATURE(sparseResidency2Samples);
    CHECK_VKDEVICE_FEATURE(sparseResidency4Samples);
    CHECK_VKDEVICE_FEATURE(sparseResidency8Samples);
    CHECK_VKDEVICE_FEATURE(sparseResidency16Samples);
    CHECK_VKDEVICE_FEATURE(sparseResidencyAliased);
    CHECK_VKDEVICE_FEATURE(variableMultisampleRate);
    CHECK_VKDEVICE_FEATURE(inheritedQueries);

    return featuresScore;
}

void instance::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
   
    //areAllFeaturesSupported(physicalDevice);

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = requestedFeatures.get();

    // createInfo.enabledExtensionCount = 0;
    
    //createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionsCount);
    //createInfo.ppEnabledExtensionNames = deviceExtensions;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedExtensions.size());
    createInfo.ppEnabledExtensionNames = requestedExtensions.data();
    

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayersSize);
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue.queue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue.queue);

}

void instance::destroyLogicalDevice() {
    vkDestroyDevice(device, nullptr);
}

//VDEVICE END

// Vulkan Memory Allocator

void instance::createAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = inst;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void instance::destroyAllocator() {
    vmaDestroyAllocator(allocator);
}

instance::svkbuffer instance::createBuffer(VkBufferCreateInfo bufferCreateInfo,VmaAllocationCreateInfo allocCreateInfo, VkDeviceSize size) {
    instance::svkbuffer buffer{};
    vmaCreateBuffer(allocator,&bufferCreateInfo,&allocCreateInfo,&buffer.buff,&buffer.alloc,&buffer.allocInfo);
    buffer.size = size;
    // buffer.inst = this;

    return buffer;
}

instance::svkbuffer instance::createComputeBuffer(VkDeviceSize size) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    return createBuffer(bufferCreateInfo, allocCreateInfo, size);
}

instance::svkbuffer instance::createStagingBuffer(VkDeviceSize size) {
    VkBufferCreateInfo stageBufferInfo{};
    stageBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stageBufferInfo.size = size;
    stageBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stageBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stageAllocInfo{};
    stageAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stageAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    return createBuffer(stageBufferInfo,stageAllocInfo,size);
}

inline instance::svkbuffer instance::createBufferStaged(VkBufferUsageFlags bufferUsage, VkDeviceSize size,const void* data,VkCommandPool commandPool) {
    instance::svkbuffer stageBuffer = createStagingBuffer(size);

    memcpy(stageBuffer.allocInfo.pMappedData,data,size);

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferUsage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    instance::svkbuffer buffer = createBuffer(bufferCreateInfo,allocCreateInfo,size);

    copyBufferToBuffer(commandPool,stageBuffer.buff,buffer.buff,size);

    destroyBuffer(stageBuffer);

    return buffer;
}

instance::svkbuffer instance::createBufferStaged(VkBufferUsageFlags bufferUsage, VkDeviceSize size,const void* data) {
    CommandPool commandPool = getCommandPool();
    return createBufferStaged(bufferUsage,size,data,commandPool.get());
}

instance::svkbuffer instance::createUniformBuffer(VkDeviceSize size) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    return createBuffer(bufferInfo,allocInfo,size);
}

void instance::copyBufferToBuffer(VkCommandPool commandPool,VkBuffer src, VkBuffer dst,VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    VkBufferCopy copyRegion; memset(&copyRegion,0,sizeof(copyRegion));
    copyRegion.srcOffset = 0; 
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    endSingleTimeCommands(commandPool,commandBuffer);
}

instance::svkimage instance::createImage(VkImageCreateInfo imageInfo, VmaAllocationCreateInfo allocInfo) {
    instance::svkimage image{};
    vmaCreateImage(allocator,&imageInfo,&allocInfo,&image.image,&image.alloc,&image.allocInfo);
    image.mipLevels = imageInfo.mipLevels;

    return std::move(image);
}

instance::svkimage instance::createComputeImage(VkImageType imageType, VkExtent3D size) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = imageType;
    imageInfo.extent = size;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    return createImage(imageInfo, allocInfo);
}

void instance::transitionImageLayout(instance::svkimage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandPool commandPool)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );


    endSingleTimeCommands(commandPool,commandBuffer);
}

void instance::transitionImageLayout(svkimage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    CommandPool commandPool = getCommandPool();
    transitionImageLayout(image,format,oldLayout,newLayout,commandPool.get());
}

void instance::copyBufferToImage(instance::svkbuffer& buffer,instance::svkimage& image, VkExtent3D extent, VkCommandPool commandPool) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer.buff,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandPool,commandBuffer);
}

void instance::copyBufferToImage(instance::svkbuffer& buffer,instance::svkimage& image, VkExtent3D extent) {
    CommandPool commandPool = getCommandPool();
    copyBufferToImage(buffer,image,extent,commandPool.get());
}

void instance::copyImageToImage(instance::svkimage& src, instance::svkimage& dst, VkExtent3D extent,VkCommandPool commandPool)  {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    VkImageCopy region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.extent = extent; 

    vkCmdCopyImage(
        commandBuffer,
        src.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandPool,commandBuffer);
}

void instance::copyImageToImage(instance::svkimage &src, instance::svkimage &dst, VkExtent3D extent) {
    CommandPool commandPool = getCommandPool();
    copyImageToImage(src,dst,extent,commandPool.get());
}

instance::svkimage instance::createImageStaged(VkImageCreateInfo imageInfo, VmaAllocationCreateInfo allocInfo, VkCommandPool commandPool, const void *data)
{
    instance::svkbuffer stageBuffer = createStagingBuffer(imageInfo.extent.width * imageInfo.extent.height * imageInfo.extent.depth * 4);

    memcpy(stageBuffer.allocInfo.pMappedData,data,imageInfo.extent.width * imageInfo.extent.height * imageInfo.extent.depth * 4);

    instance::svkimage image = createImage(imageInfo,allocInfo);

    transitionImageLayout(image,VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,commandPool);
    copyBufferToImage(stageBuffer,image,imageInfo.extent,commandPool);
    destroyBuffer(stageBuffer);
    if (imageInfo.mipLevels > 1)
        generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, {static_cast<int32_t>(imageInfo.extent.width),static_cast<int32_t>(imageInfo.extent.height),static_cast<int32_t>(imageInfo.extent.depth)}, commandPool);
    else
        transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,commandPool);
    // transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,commandPool);

    return image;
}

instance::svkimage instance::create2DImageFromFile(const char* file,uint32_t mipLevels,VkCommandPool commandPool)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    instance::svkimage image = createImageStaged(imageInfo,allocInfo,commandPool,pixels);

    stbi_image_free(pixels);

    return image;
}

instance::svkimage instance::create2DImageFromFile(const char* file, uint32_t mipLevels) {
    CommandPool commandPool = getCommandPool();
    return create2DImageFromFile(file, mipLevels, commandPool.get());
}

void instance::createImageView(svkimage& image, VkFormat format, VkImageViewType viewType, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image.image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = image.mipLevels;
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

    image.view.emplace(imageView);
}

/*@mFilter - VK_FILTER_NEAREST/VK_FILTER_LINEAR
 *@samplerAddressMode - VK_SAMPLER_ADDRESS_MODE_REPEAT/VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT/VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE/VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE/VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
*/
void instance::createSampler(svkimage& image,VkFilter mFilter, VkSamplerAddressMode samplerAddressMode)
{
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

    //TODO - anisotropy
    // VkPhysicalDeviceProperties properties{};
    // vkGetPhysicalDeviceProperties(physicalDevice, &properties); //query maxSamplerAnisotropy

    samplerInfo.anisotropyEnable = VK_FALSE;
    // samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

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
    samplerInfo.minLod = 0.0f;//0.0f;
    // samplerInfo.maxLod = 10.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE; //IDK MAN I THINK THIS IS RIGHT

    VkSampler sampler;
    if (vkCreateSampler(device, &samplerInfo, NULL, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    
    image.sampler.emplace(sampler);
}

void instance::generateMipmaps(svkimage& image, VkFormat imageFormat, VkOffset3D texSize, VkCommandPool commandPool) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texSize.x;
    int32_t mipHeight = texSize.y;
    int32_t mipDepth = texSize.z;

    for (uint32_t i = 1; i < image.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, mipDepth };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, mipDepth > 1 ? mipDepth / 2 : 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
            0, NULL,
            0, NULL,
            1, &barrier);
        
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
        if (mipDepth > 1) mipDepth /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, 
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0,
        0, NULL,
        0, NULL,
        1, &barrier);

    endSingleTimeCommands(commandPool,commandBuffer);
}

VkCommandBuffer instance::beginSingleTimeCommands(VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo; memset(&allocInfo,0,sizeof(allocInfo));
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo; memset(&beginInfo,0,sizeof(beginInfo));
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void instance::endSingleTimeCommands(VkCommandPool commandPool,VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo; memset(&submitInfo,0,sizeof(submitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (graphicsQueue.submit(1,&submitInfo,VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit queue (endSingleTimeCommands)");
    }
    graphicsQueue.waitIdle();

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void instance::destroyBuffer(instance::svkbuffer& buffer) {
    vmaDestroyBuffer(allocator,buffer.buff,buffer.alloc);
}

void instance::destroyImage(instance::svkimage& image) {
    if (image.sampler.has_value()) {
        destroySampler(image.sampler.value());
    }
    if (image.view.has_value()) {
        destroyImageView(image.view.value());
    }
    vmaDestroyImage(allocator,image.image,image.alloc);
}

void instance::destroyImageView(VkImageView imageView)
{
    vkDestroyImageView(device, imageView, NULL);
}

void instance::destroySampler(VkSampler sampler)
{
    vkDestroySampler(device, sampler, NULL);
}

// Vulkan Memory Allocator end

descriptor::allocator::pool instance::getDescriptorPool()
{
    return descriptorAllocator->getPool();
}

// Descriptor allocators
descriptor::builder instance::createDescriptorBuilder(descriptor::allocator::pool* pool) {
    return descriptor::builder::begin(&descriptorLayoutCache,pool);
}

// Descriptor end

VkDescriptorBufferInfo instance::svkbuffer::getBufferInfo()
{
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = buff;
    bufferInfo.offset = 0;
    bufferInfo.range = size;
    return bufferInfo;
}

VkDescriptorBufferInfo instance::svkbuffer::getBufferInfo(VkDeviceSize offset)
{
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = buff;
    bufferInfo.offset = offset;
    bufferInfo.range = size;
    return bufferInfo;
}

graphics::IndexBufferInfo instance::svkbuffer::getIndexBufferInfo(VkIndexType indexType)
{
    graphics::IndexBufferInfo indexBufferInfo;
    indexBufferInfo.buff = buff;
    indexBufferInfo.offset = 0;
    indexBufferInfo.type = indexType;

    switch (indexType) {
    case VK_INDEX_TYPE_UINT16:
        indexBufferInfo.count = size / sizeof(uint16_t);
        break;
    case VK_INDEX_TYPE_UINT32:
        indexBufferInfo.count = size / sizeof(uint32_t);
        break;
    default:
        throw std::runtime_error("Index type not supported");
    }

    return indexBufferInfo;
}

graphics::IndexBufferInfo instance::svkbuffer::getIndexBufferInfo(VkIndexType indexType, VkDeviceSize offset)
{
    graphics::IndexBufferInfo indexBufferInfo;
    indexBufferInfo.buff = buff;
    indexBufferInfo.offset = offset;
    indexBufferInfo.type = indexType;

    switch (indexType) {
    case VK_INDEX_TYPE_UINT16:
        indexBufferInfo.count = size / sizeof(uint16_t);
        break;
    case VK_INDEX_TYPE_UINT32:
        indexBufferInfo.count = size / sizeof(uint32_t);
        break;
    default:
        throw std::runtime_error("Index type not supported");
    }

    return indexBufferInfo;
}

// instance::svkbuffer::~svkbuffer()
// {
//     inst->destroyBuffer(*this);
// }

VkDescriptorImageInfo instance::svkimage::getImageInfo()
{
    VkDescriptorImageInfo imageInfo;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view.value();
    imageInfo.sampler = sampler.value();
    return imageInfo;
}




//CommandPool class
instance::CommandPool::CommandPool(instance &inst, const VkCommandPool commandPool, const int id)
    : inst(inst), id(id), pool(commandPool)
{}

instance::CommandPool::~CommandPool()
{
    returnPool();
}

VkCommandPool instance::CommandPool::get()
{
    return pool;
}

void instance::CommandPool::returnPool()
{
    if (this->pool == VK_NULL_HANDLE) 
        return;
    this->pool = VK_NULL_HANDLE;
    static threadpool* pool = threadpool::get_instance();
    instance& inst = this->inst;
    int id = this->id;
    pool->add_task([&inst,id](){
        inst.returnCommandPool(id);
    });
}
//CommandPool class end


} // namespace svklib

#endif // SVKLIB_INSTANCE_CPP

