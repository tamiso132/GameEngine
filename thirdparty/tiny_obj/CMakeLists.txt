
add_library(tinyobj STATIC)

target_include_directories(tinyobj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(tinyobj PRIVATE
    tiny_obj_loader.cc
    tiny_obj_loader.h
)

