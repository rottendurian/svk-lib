#ifndef SVKLIB_RENDERER_HPP
#define SVKLIB_RENDERER_HPP

#include "pch.hpp"
#include "svk_instance.hpp"
#include "svk_swapchain.hpp"
#include "svk_pipeline.hpp"

#include <functional>


namespace svklib {

namespace queue {

class sync {
    friend class renderer;
public:
    sync(instance& inst,const int count);
    ~sync();

    void syncFrame(const int frame);
    VkResult acquireNextImage(const int frame, VkSwapchainKHR swapChain, uint32_t& imageIndex);
    void submitFrame(const int frame, const uint32_t commandBufferCount, VkCommandBuffer* commandBuffers, instance::svkqueue& queue);
    VkResult presentFrame(const int frame, const uint32_t imageIndex, VkSwapchainKHR swapChain, instance::svkqueue &queue);

private:
    void createSyncObjects();
    void destroySyncObjects();

    instance& inst;

    void* buffer;
    const int count;
    VkFence* inFlightFence;
    VkSemaphore* imageAvailableSemaphore;
    VkSemaphore* renderFinishedSemaphore;
};

} // namespace queue

class renderer {
public:
    renderer(instance& inst, swapchain& swap, graphics::pipeline& pipe);
    ~renderer();

    void drawFrame();
    std::function<void(uint32_t)> updateUniforms = nullptr;

private:
    //references
    instance& inst;
    swapchain& swap;
    graphics::pipeline& pipe;
    //references end

    queue::sync sync;

public:
    uint32_t currentFrame = 0;

private:
    void recordDrawCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

};





} // namespace svklib


#endif // SVKLIB_RENDERER_HPP