#ifndef SVKLIB_WINDOW_HPP
#define SVKLIB_WINDOW_HPP

#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#include "pch.hpp"

namespace svklib {

class instance;

class window {
    friend class instance;
public:
    window(const char* title, int width, int height);
    ~window();

    bool shouldClose();
    VkSurfaceKHR createSurface(VkInstance instance);
    bool frameBufferResized();

    void setWindowUserPointer(void* ptr);
    void* getWindowUserPointer();

    GLFWwindow* win;
private:
    bool resized = false;
    void* internalPtr;
    
    static void frameBufferResizeCallBack(GLFWwindow* glfwwindow, int width, int height);

};

} // namespace svklib

#endif // SVKLIB_WINDOW_HPP