#pragma once

#include "../vk_engine.h"
#include "../vk_types.h"
#include <cstdint>

ResourceBuilder init_mesh();


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



namespace TextureHelper {

    void load_texture_array(const char *filePath, uint32_t gridLength, AllocatedImage &textureArray, VkImageView *view);
    void load_cube_map(AllocatedImage *cubeMapImage, VkImageView *view);
} // namespace TextureHelper
