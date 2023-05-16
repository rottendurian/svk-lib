#ifndef _SVKLIB_FORWARD_DECLARATIONS_HPP
#define _SVKLIB_FORWARD_DECLARATIONS_HPP

namespace svklib {

class window;

namespace descriptor {
    struct allocator_pool;
    class allocator;
    class layout_cache;
    class builder;
}

class threadpool;

class instance;
class swapchain;

namespace graphics {
    class pipeline;
    struct IndexBufferInfo {
        VkBuffer buff;
        VkDeviceSize offset;
        uint32_t count;
        VkIndexType type;
    };
}

namespace compute {
    class pipeline;
}

class renderer;

} // namespace svklib

#endif // _SVKLIB_FORWARD_DECLARATIONS_HPP