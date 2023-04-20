cmake_minimum_required(VERSION 3.22.0)

project(vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} src/main.cpp src/camera.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

add_subdirectory(svklib)

target_link_libraries(${PROJECT_NAME} svklib)

target_include_directories(${PROJECT_NAME} 
    PUBLIC svklib
    PUBLIC dep/glm
    PUBLIC src
)

