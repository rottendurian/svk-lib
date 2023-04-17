#include <svklib.hpp>
#include <iostream>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //VK_VERTEX_INPUT_RATE_INSTANCE for instanced rendering

        return std::vector<VkVertexInputBindingDescription>{bindingDescription};
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
        attributeDescriptions.resize(3);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    float tessLevel = 1.f;
};

static float tessLevel = 1.f;

void updateUniformBuffer(std::vector<svklib::instance::svkbuffer>& uniformBuffers,uint32_t currentFrame,float time, gllib::camera& camera) {
    UniformBufferObject ubo{};
    ubo.model = glm::identity<glm::mat4>();
    ubo.model = glm::rotate(ubo.model, glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f));
    // ubo.model = glm::scale(ubo.model, glm::vec3(20.0f));
    camera.update();
    ubo.view = camera.get_view();
    ubo.proj = camera.get_proj();
    ubo.tessLevel = tessLevel;

    memcpy(uniformBuffers[currentFrame].allocInfo.pMappedData, &ubo, sizeof(UniformBufferObject));
}

static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = 0.f, lastY = 0.f;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    svklib::window* win = static_cast<svklib::window*>(glfwGetWindowUserPointer(window));
    gllib::camera* camera = static_cast<gllib::camera*>(win->getWindowUserPointer());
    camera->rotate(xoffset, yoffset, 0.01f);
}

int main() {

    float m = 10.f;
    
    const std::vector<Vertex> vertices = {
        {{-0.5f*m, -0.5f*m, 0.0f*m}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f*m, -0.5f*m, 0.0f*m}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f*m, 0.5f*m, 0.0f*m}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f*m, 0.5f*m, 0.0f*m}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        // {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        // {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        // {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        // {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        // 4, 5, 6, 6, 7, 4
    };

    gllib::camera camera(45.f,0.001f,1000.f,800,600,glm::vec3(0.0f,0.0f,0.0f));

    svklib::threadpool* threadPool = svklib::threadpool::get_instance();

    try {
        auto start = std::chrono::high_resolution_clock::now();
    
        svklib::window win("Vulkan", 800, 600);
        svklib::instance inst(win);
        svklib::swapchain swap(inst,3);
        svklib::graphics::pipeline pipeline(inst,swap);
        // svklib::compute::pipeline computePipeline(inst);
        svklib::renderer render(inst,swap,pipeline);

        std::atomic_bool textureImageComplete(false);
        svklib::instance::svkimage textureImage;
        threadPool->add_task([&](){
            textureImage = inst.create2DImageFromFile("res/textures/perlin.bmp",4);
            // textureImage = inst.create2DImageFromFile("res/textures/bluetreesforest.jpg",5,tempCommandPool);
            inst.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
            inst.createSampler(textureImage,VK_FILTER_LINEAR,VK_SAMPLER_ADDRESS_MODE_REPEAT);
        },&textureImageComplete);

        win.setWindowUserPointer(&camera);
        glfwSetCursorPosCallback(win.win, mouse_callback);
        glfwSetInputMode(win.win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        auto vertexBindingDescriptions = Vertex::getBindingDescription(); 
        auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
        pipeline.buildVertexInputState(vertexBindingDescriptions, vertexAttributeDescriptions);

        pipeline.buildShader("res/shaders/simple_shader.vert",VK_SHADER_STAGE_VERTEX_BIT);
        pipeline.buildShader("res/shaders/simple_shader.tesc",VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
        pipeline.buildShader("res/shaders/simple_shader.tese",VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
        pipeline.buildShader("res/shaders/simple_shader.frag",VK_SHADER_STAGE_FRAGMENT_BIT);
        
        pipeline.buildInputAssembly(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
        pipeline.buildDynamicState({VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR});
        pipeline.buildRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipeline.buildMultisampling();
        pipeline.buildDepthStencil();
        pipeline.buildColorBlendAttachment(false);
        pipeline.buildColorBlending();
        pipeline.buildTessellationState(3);
        pipeline.addToBuildQueue([&](){
            pipeline.buildViewport(swap.swapChainExtent.width, swap.swapChainExtent.height);
            pipeline.buildScissor({0,0}, {swap.swapChainExtent.width,swap.swapChainExtent.height});
            pipeline.buildViewportState();
        });
        
        svklib::instance::svkbuffer vertexBuffer = inst.createBufferStaged(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.size()*sizeof(Vertex), vertices.data());
        svklib::instance::svkbuffer indexBuffer = inst.createBufferStaged(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.size()*sizeof(uint16_t), indices.data());
        
        std::vector<svklib::instance::svkbuffer> uniformBuffers{}; uniformBuffers.resize(swap.framesInFlight);
        std::vector<VkDescriptorBufferInfo> bufferInfo{}; bufferInfo.resize(swap.framesInFlight);
        for (size_t i = 0; i < swap.framesInFlight; i++) {
            uniformBuffers[i] = inst.createUniformBuffer(sizeof(UniformBufferObject));
            bufferInfo[i] = uniformBuffers[i].getBufferInfo();
        }

        pipeline.vertexBuffers.push_back(vertexBuffer.buff);
        pipeline.vertexBufferOffsets.push_back(0);

        pipeline.indexBufferInfo = indexBuffer.getIndexBufferInfo(VK_INDEX_TYPE_UINT16);

        pipeline.descriptorSets.resize(1);
        pipeline.descriptorSets[0].resize(swap.framesInFlight);
        pipeline.descriptorSetLayouts.resize(1);

        svklib::descriptor::allocator::pool descriptorPool{inst.getDescriptorPool()};
        svklib::descriptor::builder descriptorBuilder = inst.createDescriptorBuilder(&descriptorPool);
        descriptorBuilder.bind_buffer(0,bufferInfo.data(),VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_ALL);
        
        while (!textureImageComplete.load()) {
            std::this_thread::yield();
        }
        VkDescriptorImageInfo imageInfo = textureImage.getImageInfo();

        descriptorBuilder.bind_image(1,&imageInfo,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_ALL);
        for (uint32_t i = 0; i < swap.framesInFlight; i++) {
            descriptorBuilder.update_buffer(0,&bufferInfo[i]);
            descriptorBuilder.build(pipeline.descriptorSets[0][i],pipeline.descriptorSetLayouts[0]);
        }

        // glm::vec4 pushConstantData = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        // pipeline.pushConstantRange.size = sizeof(glm::vec4);
        // pipeline.pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        // pipeline.pushConstantRange.offset = 0;
        // pipeline.pushConstantData = &pushConstantData;
        // pipeline.pushConstantCount = 1;
        
        pipeline.buildPipeline();

        auto end = std::chrono::high_resolution_clock::now();
        
        double svktime = std::chrono::duration<double>(end - start).count();

        std::cout << "Time taken to build pipeline: " << svktime << "s" << std::endl;

        double prevTime = svktime;
        double time = 0.;
        double deltaTime = 0.;

        auto updateDeltaTime = [&]() {
            end = std::chrono::high_resolution_clock::now();
            time = std::chrono::duration<double>(end - start).count();
            deltaTime = time - prevTime;
            prevTime = time;
        };

        auto updateUniforms = [&](uint32_t currentFrame) {
            updateUniformBuffer(uniformBuffers,currentFrame, time, camera);
        };
        render.updateUniforms = updateUniforms;

        while (!win.shouldClose()) {
            glfwPollEvents();

            updateDeltaTime();

            if (glfwGetKey(win.win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(win.win, true);
            }
            if (glfwGetKey(win.win, GLFW_KEY_W) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::forward, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_S) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::backward, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_A) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::left, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_D) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::right, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_SPACE) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::down, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                camera.move(gllib::camera::movement::up, deltaTime);
            }
            if (glfwGetKey(win.win, GLFW_KEY_UP) == GLFW_PRESS) {
                tessLevel += 0.1f;
                tessLevel = std::min(tessLevel, 64.f);
            }
            if (glfwGetKey(win.win, GLFW_KEY_DOWN) == GLFW_PRESS) {
                tessLevel -= 0.1f;
                tessLevel = std::max(tessLevel, 1.f);
            }
            
            std::string title("Vulkan FPS: "); 
            title += std::to_string(1./deltaTime);
            glfwSetWindowTitle(win.win,title.c_str());

            
            render.drawFrame();
            
        }

        inst.waitForDeviceIdle();
        
        inst.destroyImage(textureImage);
        inst.destroyBuffer(vertexBuffer);
        inst.destroyBuffer(indexBuffer);
        
        for (auto& uniformBuffer : uniformBuffers) {
            inst.destroyBuffer(uniformBuffer);
        }

        
    }
    catch (std::runtime_error &e) {
        std::cout << "Error: " << e.what() << std::endl;
        return -1;
    }
    

    return 0;
}

