#pragma once

#include "common/common_types.h"

#include <array>
#include <cstdint>
#include <glm/fwd.hpp>

const uint32_t CHUNK_WIDTH_LENGTH = 32;
const uint32_t CHUNK_HEIGHT       = 256;

struct Player {
    glm::vec3 position;
};

struct alignas(16) ChunkPushConstant {
    glm::mat4 startChunk;
    glm::vec3 playerPosition;
};

struct alignas(16) ChunkRenderData {
    uint32_t chunkIndex = 0;

    // How many heights that will be rendered
    uint32_t renderBlock = 0;
};

// relative to the world

struct MinecraftBlock {
    uint64_t blockId;
};

struct Chunk {

    /* Chunk meta data*/
    glm::vec3 position;

    std::array<uint32_t, CHUNK_WIDTH_LENGTH * CHUNK_WIDTH_LENGTH * CHUNK_HEIGHT> blocks;
};