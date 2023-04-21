cmake_minimum_required(VERSION 3.22.0)

project(svklib)

add_library(${PROJECT_NAME} STATIC
    pch.cpp
    svklib.cpp
    svk_window.cpp
    svk_descriptor.cpp
    svk_instance.cpp
    svk_swapchain.cpp
    svk_pipeline.cpp
    svk_renderer.cpp
    svk_shader.cpp
    svk_threadpool.cpp
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_precompile_headers(${PROJECT_NAME} PUBLIC pch.hpp)

find_package(Vulkan REQUIRED)

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC ${Vulkan_LIBRARIES}
)

message("glslang libraries ${GLSLANG_LIBRARIES}")

find_package(glslang CONFIG REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} 
   PUBLIC 
     glslang::glslang 
     glslang::SPIRV 
     glslang::glslang-default-resource-limits
)

add_subdirectory(dep/glfw)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC glfw
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC dep/glfw/include
    PUBLIC dep/stp
    PUBLIC dep/stb
    PUBLIC dep
)
