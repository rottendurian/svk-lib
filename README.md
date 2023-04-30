A simple vulkan library with multi-threading support built in. 

Built using vcpkg, setup.py will setup vcpkg for you

This project also relies on the VulkanSDK which should locatable by FindVulkan in cmake

Example of setting up the cmake file:

cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

cmake --build build --parallel 16
