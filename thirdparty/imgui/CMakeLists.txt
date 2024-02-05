file(GLOB IMGUI_SOURCES *.cpp)
file(GLOB IMGUI_HEADERS *.h)

find_package(SDL2 REQUIRED)

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(imgui PRIVATE
    imgui.h
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_impl_vulkan.cpp
    imgui_impl_sdl2.cpp
)

target_include_directories(imgui PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(imgui PRIVATE ${SDL2_LIBRARIES})
