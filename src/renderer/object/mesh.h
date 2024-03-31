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

    void       init_texture();
    GPUTexture get_texture(Block::Type blockType);
    uint32_t   get_vertices_size();

} // namespace Block

struct PNGContext {
    size_t   texWidth, texHeight, pixelSize;
    uint8_t *pixels;
};

namespace TextureHelper {

    PNGContext load_png(const char *filePath);
} // namespace TextureHelper
