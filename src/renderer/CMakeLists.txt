
add_subdirectory(util)
add_subdirectory(object)
add_subdirectory(common)

add_sources(
    vk_engine.h
    vk_engine.cpp
    vk_types.h
    mesh.cpp
    mesh.h
    resource.cpp
    vk_resource.h
    pipeline.cpp
    pipeline.h
)

include_this()


