find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

find_package(fmt REQUIRED)

find_package(glm REQUIRED)

# Add source to this project's executable.

find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_executable(VulkanGuide
main.cpp
vk_engine.cpp
vk_engine.h
vk_types.h
vk_mesh.cpp
vk_mesh.h
vk_textures.cpp
vk_textures.h
vk_initializers.cpp
vk_initializers.h
camera/camera.cpp
camera/camera.h
vk_descriptors.cpp
vk_descriptors.h
helper.h
helper.cpp
vk_create.h
vk_create.cpp
)

target_include_directories(VulkanGuide PRIVATE ${Vulkan_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})

target_link_libraries(VulkanGuide PRIVATE imgui fastgltf ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS} vk-bootstrap fmt::fmt GPUOpen::VulkanMemoryAllocator tinyobj stb_image)