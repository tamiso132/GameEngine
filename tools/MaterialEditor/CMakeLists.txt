# project_root/CMakeLists.txt

# Minimum required version of CMake
cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_STANDARD 20)

# Project name
project(MaterialEditor)
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_SOURCE_DIR}/../..")

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

macro(add_sources)
    # Get the relative path from the project root to the current source directory
    file(RELATIVE_PATH RELATIVE_PATH_TO_PROJECT ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    
    # Iterate over the source files and add them to the list
    foreach(SOURCE_FILE ${ARGV})
        # Append to the list of sources with the relative path
        list(APPEND SOURCES_FILES_GAME_ENGINE ${RELATIVE_PATH_TO_PROJECT}/${SOURCE_FILE})
    endforeach()
    set(SOURCES_FILES_GAME_ENGINE ${SOURCES_FILES_GAME_ENGINE} PARENT_SCOPE)
endmacro()

macro(include_this)
    file(RELATIVE_PATH RELATIVE_PATH_TO_PROJECT ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    list(APPEND INCLUDE_DIRECTORIES_ENGINE "\n${RELATIVE_PATH_TO_PROJECT}")
    set(INCLUDE_DIRECTORIES_ENGINE ${INCLUDE_DIRECTORIES_ENGINE} PARENT_SCOPE)
endmacro()

add_subdirectory(../../thirdparty/vma  ${CMAKE_SOURCE_DIR}/build/thirdparty/vma)
add_subdirectory(../../thirdparty/imgui ${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/imgui)
add_subdirectory(../../thirdparty/simdjson ${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/simdjson)
add_subdirectory(../../thirdparty/fastgltf  ${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/fastgltf)
add_subdirectory(../../thirdparty/vk-bootstrap  ${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/vk-bootstrap)
add_subdirectory(../../thirdparty/stb_image  ${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/stb_image)

add_subdirectory(../../src/renderer/common  ${CMAKE_SOURCE_DIR}/build/src/renderer/common)
add_subdirectory(../../src/renderer/util  ${CMAKE_SOURCE_DIR}/build/src/renderer/util)
add_subdirectory(../../src/camera  ${CMAKE_SOURCE_DIR}/build/src/camera)

add_subdirectory(src)

add_compile_options(-w)

add_executable(MaterialEditor ${SOURCES_FILES_GAME_ENGINE})


target_include_directories(MaterialEditor PRIVATE ${Vulkan_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ../../src/renderer/)


target_link_libraries(MaterialEditor PRIVATE imgui fastgltf ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS} vk-bootstrap fmt::fmt stb_image vma)

target_include_directories(MaterialEditor PRIVATE ${INCLUDE_DIRECTORIES_ENGINE})
target_include_directories(MaterialEditor PRIVATE thirdparty/stb_image thirdparty/fastgltf/include thirdparty/imgui)
