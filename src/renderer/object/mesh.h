#pragma once

#include "../vk_engine.h"
#include "../vk_types.h"
#include <cstdint>

void init_mesh();

enum MeshType {
    Mesh_Quad,
    Mesh_Cube,
};

namespace Block {

    enum Type {
        ACACIA_TREE = 0,
        ACACIA_PLANKS,
        ANDESITE,
        WOOD_BARREL,
        BIRCH_TREE,
        BIRCH_PLANKS,
        FLOWER_RED,
    };

    void init_texture();
    GPUTexture get_texture(Block::Type blockType);
    uint32_t get_vertices_size();

} // namespace Block

namespace Quad {
    static AllocatedBuffer get_vertices();
    AllocatedBuffer get_screen_vertices();
} // namespace Quad

namespace Cube {
    AllocatedBuffer get_vertices();
    uint32_t get_vertices_size();
    const AllocatedBuffer get_vertices_buffer();
} // namespace Cube

namespace TextureHelper {

    void load_texture_array(const char *filePath, uint32_t gridLength, AllocatedImage &textureArray, VkImageView *view);
    void load_cube_map(AllocatedImage *cubeMapImage, VkImageView *view);
} // namespace TextureHelper
