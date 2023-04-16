#ifndef SVKLIB_WINDOW_CPP
#define SVKLIB_WINDOW_CPP

#include "svk_window.hpp"

namespace svklib {

void window::frameBufferResizeCallBack(GLFWwindow* glfwwindow, int width, int height) {
    window* win = static_cast<window*>(glfwGetWindowUserPointer(glfwwindow));
    win->resized = true;
}

window::window(const char* name, int width, int height) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    win = glfwCreateWindow(width, height, name, nullptr, nullptr);

    glfwSetWindowUserPointer(win, this);
    glfwSetFramebufferSizeCallback(win, frameBufferResizeCallBack);
}

window::~window() {
    glfwDestroyWindow(win);
    glfwTerminate();
}

bool window::shouldClose() {
    return glfwWindowShouldClose(win);
}

VkSurfaceKHR window::createSurface(VkInstance instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, win, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return surface;
}

bool window::frameBufferResized() {
    if (resized == true) {
        resized = false;
        return true;
    }
    return resized;
}

void window::setWindowUserPointer(void *ptr) {
    internalPtr = ptr;
}

void* window::getWindowUserPointer()
{
    return internalPtr;
}

} // namespace svklib

#endif // SVKLIB_WINDOW_CPP