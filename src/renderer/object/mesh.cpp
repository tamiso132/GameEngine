#include "mesh.h"
#include "../vk_mesh.h"

ResourceBuilder init_mesh() {
    ResourceBuilder builder;

    const std::vector<VertexTemp> quadVertices = {
        // Vertex 1
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 0}, 0},

        // Vertex 2
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 0}, 0},

        // Vertex 3
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 1}, 0},

        // Vertex 4
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 1}, 0},
    };

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

    const std::vector<VertexQuadScreen> quadScreenVertices = {
        // Vertex 1
        {{0, 0}},

        // Vertex 2
        {{1, 0}},

        // Vertex 3
        {{0, 1}},

        // Vertex 4
        {{1, 1}},
    };

    builder.add_buffer(quadVertices.size(), BufferType::Vertex, "v_quad", MemoryType::GpuOnly, quadVertices.data(), quadVertices.size(), true);
    builder.add_buffer(cubeVertices.size(), BufferType::Vertex, "v_cube", MemoryType::GpuOnly, cubeVertices.data(), quadScreenVertices.size(), true);
    builder.add_buffer(quadScreenVertices.size(), BufferType::Vertex, "v_quad_screen", MemoryType::GpuOnly, quadScreenVertices.data(), quadVertices.size(), true);

    return builder;
}