# project_root/CMakeLists.txt

# Minimum required version of CMake
cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Project name
project(VulkanGuide)
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")



# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)

# Set the default build type if not specified
# Include the source directory



add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/simdjson)
add_subdirectory(thirdparty/fastgltf)
add_subdirectory(thirdparty/vk-bootstrap)
add_subdirectory(thirdparty/tiny_obj)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(src)

add_compile_options(-w)

target_include_directories(VulkanGuide PRIVATE src)
target_include_directories(VulkanGuide PRIVATE thirdparty/stb_image thirdparty/fastgltf/include thirdparty/imgui)

