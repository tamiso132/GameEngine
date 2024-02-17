cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_STANDARD 20)

# Project name
project(AtlasGenerator)


add_subdirectory(../../thirdparty/stb_image)
add_executable(AtlasGenerator src/main.cpp)

target_link_libraries(AtlasGenerator PRIVATE stb_image)
target_include_directories(AtlasGenerator PRIVATE ../../thirdparty/imgui)