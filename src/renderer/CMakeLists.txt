
add_subdirectory(util)
add_subdirectory(object)

add_sources(
    vk_engine.h
    vk_engine.cpp
    vk_types.h
    vk_mesh.cpp
    vk_mesh.h
    vk_create.h
    vk_create.cpp
    renderer.h
    renderer.cpp
)

include_this()


