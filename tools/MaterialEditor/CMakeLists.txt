# project_root/CMakeLists.txt

# Minimum required version of CMake
cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_STANDARD 20)

# Project name
project(VulkanGuide)
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}")

find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

find_package(fmt REQUIRED)

find_package(glm REQUIRED)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)

# Set the default build type if not specified
# Include the source directory
set(SOURCES_FILES_GAME_ENGINE)
set(INCLUDE_DIRECTORIES_ENGINE)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(thirdparty/imgui)
add_subdirectory(thirdparty/simdjson)
add_subdirectory(thirdparty/fastgltf)
add_subdirectory(thirdparty/vk-bootstrap)
add_subdirectory(thirdparty/tiny_obj)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(src)

add_compile_options(-w)


add_executable(VulkanGuide ${SOURCES_FILES_GAME_ENGINE})


target_include_directories(VulkanGuide PRIVATE ${Vulkan_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} thirdparty/vma)

target_link_libraries(VulkanGuide PRIVATE imgui fastgltf ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS} vk-bootstrap fmt::fmt tinyobj stb_image)

target_include_directories(VulkanGuide PRIVATE ${INCLUDE_DIRECTORIES_ENGINE})
target_include_directories(VulkanGuide PRIVATE thirdparty/stb_image thirdparty/fastgltf/include thirdparty/imgui)
