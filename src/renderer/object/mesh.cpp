#include "mesh.h"
#include "../util/helper.h"
#include "../vk_mesh.h"

#include <cstdint>
#include <cstring>

namespace Quad {
    AllocatedBuffer quadVerticesBuffer;
    uint32_t verticesSize;
    void init_quad_vertices() {

        std::vector<VertexTemp> quadVertices = {
            // Vertex 1
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 0}, 0},

            // Vertex 2
            {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 0}, 0},

            // Vertex 3
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 1}, 0},

            // Vertex 4
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 1}, 0},
        };

        verticesSize = quadVertices.size();
        const size_t vertexBufferSize = quadVertices.size() * sizeof(VertexTemp);

        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = vertexBufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(Helper::allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

        // Copy vertex data to staging buffer
        void *stagingData;
        vmaMapMemory(Helper::allocator, stagingBuffer._allocation, &stagingData);
        memcpy(stagingData, quadVertices.data(), vertexBufferSize);
        vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);

        // Vertex buffer for GPU usage
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = vertexBufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo vertexAllocInfo = {};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateBuffer(Helper::allocator, &vertexBufferInfo, &vertexAllocInfo, &quadVerticesBuffer._buffer, &quadVerticesBuffer._allocation, nullptr));

        Helper::copy_buffer(stagingBuffer._buffer, quadVerticesBuffer._buffer, vertexBufferSize);

        vmaDestroyBuffer(Helper::allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }
} // namespace Quad

namespace Cube {

    AllocatedBuffer cubeVerticesBuffer;
    uint32_t verticesSize = 0;

    void init_cube_vertices() {

        const std::vector<VertexTemp> cubeVertices = {                                                             // Right face
                                                      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0},   // Top left
                                                      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 0},  // Top Right
                                                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0}, // Bottom Right
                                                      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 0}, // Bottom Right
                                                      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 0},  // Bottom Left
                                                      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0},   // Top left

                                                      // Left face
                                                      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 1},   // top-right
                                                      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 1}, // bottom-left
                                                      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, 1},  // bottom-right
                                                      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, 1}, // bottom-left
                                                      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, 1},   // top-right
                                                      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 1},  // top-left

                                                      // Top face
                                                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, 2},
                                                      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, 2},
                                                      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, 2},
                                                      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, 2},
                                                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, 2},
                                                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, 2},

                                                      // Bottom face
                                                      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, 3},
                                                      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, 3},
                                                      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, 3},
                                                      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, 3},
                                                      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, 3},
                                                      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, 3},

                                                      // Front face
                                                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 5},
                                                      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 5},
                                                      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, 5},
                                                      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 5},
                                                      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, 5},
                                                      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 5},

                                                      // Back face
                                                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, 4},
                                                      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, 4},
                                                      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, 4},
                                                      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, 4},
                                                      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, 4},
                                                      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, 4}};

        verticesSize = cubeVertices.size();

        const size_t vertexBufferSize = cubeVertices.size() * sizeof(VertexTemp);

        VkBufferCreateInfo stagingBufferInfo = {};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = vertexBufferSize;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo = {};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

        AllocatedBuffer stagingBuffer;
        VK_CHECK(vmaCreateBuffer(Helper::allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

        // Copy vertex data to staging buffer
        void *stagingData;
        vmaMapMemory(Helper::allocator, stagingBuffer._allocation, &stagingData);
        memcpy(stagingData, cubeVertices.data(), vertexBufferSize);
        vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);

        // Vertex buffer for GPU usage
        VkBufferCreateInfo vertexBufferInfo = {};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = vertexBufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo vertexAllocInfo = {};
        vertexAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VK_CHECK(vmaCreateBuffer(Helper::allocator, &vertexBufferInfo, &vertexAllocInfo, &cubeVerticesBuffer._buffer, &cubeVerticesBuffer._allocation, nullptr));

        Helper::copy_buffer(stagingBuffer._buffer, cubeVerticesBuffer._buffer, vertexBufferSize);

        vmaDestroyBuffer(Helper::allocator, stagingBuffer._buffer, stagingBuffer._allocation);
    }

    const AllocatedBuffer get_vertices_buffer() { return cubeVerticesBuffer; };
    uint32_t get_vertices_size() { return verticesSize; }
} // namespace Cube

void init_mesh() {
    Quad::init_quad_vertices();
    Cube::init_cube_vertices();
}