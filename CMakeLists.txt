set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_minimum_required(VERSION 3.22.0)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

set(SVKLIB_BUILD_EXAMPLE "ON")

if(SVKLIB_BUILD_EXAMPLE STREQUAL "ON")
    project(vulkan)

    add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/example/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/example/camera.cpp)

    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

    add_subdirectory(svklib)

    target_link_libraries(${PROJECT_NAME} svklib)

    target_include_directories(${PROJECT_NAME} 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/svklib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dep/glm
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
else()
    add_subdirectory(svklib)
endif()

