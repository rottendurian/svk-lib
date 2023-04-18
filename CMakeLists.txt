cmake_minimum_required(VERSION 3.22.0)

project(vulkan)

add_executable(${PROJECT_NAME} main.cpp camera.cpp)

# set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# string(COMPARE EQUAL ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} PROJECT_IS_TOP_LEVEL)

# if(PROJECT_IS_TOP_LEVEL)
#     find_package(Vulkan REQUIRED)
#     include_directories(${Vulkan_INCLUDE_DIR})
# endif()

add_subdirectory(svklib)

target_link_libraries(${PROJECT_NAME} svklib)

target_include_directories(${PROJECT_NAME} 
    PUBLIC svklib
    PUBLIC dep/glm
)

