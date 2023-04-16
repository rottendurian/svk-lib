#ifndef SVKLIB_RENDERER_HPP
#define SVKLIB_RENDERER_HPP

#include "pch.hpp"
#include "svk_instance.hpp"
#include "svk_swapchain.hpp"
#include "svk_pipeline.hpp"

#include <functional>


namespace svklib {

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

    // instance::CommandPool* commandPool;
    
    //record draw
    // std::vector<VkCommandBuffer> drawCommandBuffer; //drawing command buffer
    // void createDrawCommandBuffer(); 

    //draw frame
    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderFinishedSemaphore;
    std::vector<VkFence> inFlightFence;

public:
    uint32_t currentFrame = 0;

private:
    void createSyncObjects();
    void recordDrawCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

};





} // namespace svklib


#endif // SVKLIB_RENDERER_HPP