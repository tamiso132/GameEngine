#include "mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vkinit {
    VkVertexInputAttributeDescription vertex_attribute_input_info(size_t location, VkFormat format, size_t offset) {
        VkVertexInputAttributeDescription attributeInfo{};

        attributeInfo.location = location;
        attributeInfo.format   = format;
        attributeInfo.offset   = offset;

        return attributeInfo;
    }
} // namespace vkinit

VertexInputDescription VertexBlock::vertex_input_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding                         = 0;
    mainBinding.stride                          = sizeof(VertexBlock);
    mainBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription positionAttribute = vkinit::vertex_attribute_input_info(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexBlock, position));
    VkVertexInputAttributeDescription normalAttribute   = vkinit::vertex_attribute_input_info(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexBlock, normal));
    VkVertexInputAttributeDescription uvAttribute       = vkinit::vertex_attribute_input_info(2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexBlock, uv));
    VkVertexInputAttributeDescription faceAttribute     = vkinit::vertex_attribute_input_info(3, VK_FORMAT_R32_UINT, offsetof(VertexBlock, faceIndex));

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(uvAttribute);
    description.attributes.push_back(faceAttribute);
    description.bindings.push_back(mainBinding);

    return description;
}

VertexInputDescription VertexQuadScreen::vertex_input_description() {
    VertexInputDescription description;

    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.stride                          = sizeof(VertexQuadScreen);
    mainBinding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription uvAttribute = vkinit::vertex_attribute_input_info(2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexQuadScreen, uv));

    description.attributes.push_back(uvAttribute);
    description.bindings.push_back(mainBinding);

    return description;
}

void Mesh::init_mesh(ResourceContext &context) {
    ResourceBuilder builder = {};

    const std::vector<VertexBlock> quadVertices = {
        // Vertex 1
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 0}, 0},

        // Vertex 2
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 0}, 0},

        // Vertex 3
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0, 1}, 0},

        // Vertex 4
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1, 1}, 0},
    };

    const std::vector<VertexBlock> cubeVertices = {                                                             // Right face
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

    builder.add_buffer(cubeVertices.size(), BufferType::Vertex, "v_cube", MemoryType::GpuOnly, cubeVertices.data(), quadScreenVertices.size());
    builder.add_buffer(quadScreenVertices.size(), BufferType::Vertex, "v_quad_screen", MemoryType::GpuOnly, quadScreenVertices.data(), quadVertices.size());
    builder.add_buffer(quadVertices.size(), BufferType::Vertex, "v_quad", MemoryType::GpuOnly, quadVertices.data(), quadVertices.size());
    ci_blueprintIndex = context.build_resource_blueprint(builder.get_blueprint());

    auto quadContext   = MeshContext(quadVertices.size(), context.get_buffer_by_index(ci_blueprintIndex, (uint64_t)MeshType::CubeBlock).alloc, VertexBlock::vertex_input_description);
    auto cubeContext   = MeshContext(cubeVertices.size(), context.get_buffer_by_index(ci_blueprintIndex, (uint64_t)MeshType::QuadBlock).alloc, VertexBlock::vertex_input_description);
    auto screenContext = MeshContext(cubeVertices.size(), context.get_buffer_by_index(ci_blueprintIndex, (uint64_t)MeshType::QuadScreen).alloc, VertexQuadScreen::vertex_input_description);

    ci_meshes.insert({MeshType::CubeBlock, cubeContext});
    ci_meshes.insert({MeshType::QuadBlock, quadContext});
    ci_meshes.insert({MeshType::QuadScreen, screenContext});
}

MeshContext Mesh::get_mesh(MeshType type) {
    assert(ci_meshes.contains(type));
    return ci_meshes[type];
}

PNGContext load_png(const char *filePath) {
    PNGContext context;

    int texWidth, texHeight, texChannels;

    auto fullpath = std::string(PROJECT_ROOT_PATH) + "/" + filePath;

    stbi_uc *pixels = stbi_load(fullpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (pixels == nullptr) {
        std::cout << "Failed to load texture file " << fullpath << std::endl;
        throw std::runtime_error("failed to load texture!");
    }

    context.pixels    = pixels;
    context.pixelSize = STBI_rgb_alpha;
    context.texHeight = texHeight;
    context.texWidth  = texWidth;

    return context;
}