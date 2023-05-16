#ifndef SVKLIB_WINDOW_HPP
#define SVKLIB_WINDOW_HPP

namespace svklib {

class instance;

class window {
    friend class instance;
public:
    window(const char* title, int width, int height);
    ~window();

    window(const window&) = delete;
    window& operator=(const window&) = delete;

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
