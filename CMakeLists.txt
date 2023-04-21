set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

cmake_minimum_required(VERSION 3.22.0)

project(vulkan)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/camera.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

add_subdirectory(svklib)

target_link_libraries(${PROJECT_NAME} svklib)

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/svklib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dep/glm
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

