cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_STANDARD 20)
set(STB_IMAGE_BINARY_DIR ${CMAKE_BINARY_DIR}/build)
add_compile_definitions(PROJECT_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
# Project name
project(AtlasGenerator)

add_subdirectory(../../thirdparty/stb_image ${STB_IMAGE_BINARY_DIR})
add_executable(AtlasGenerator src/main.cpp)

target_link_libraries(AtlasGenerator PRIVATE stb_image)
target_include_directories(AtlasGenerator PRIVATE ../../thirdparty/stb_image)