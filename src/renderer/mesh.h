#pragma once

#include "vk_resource.h"
#include "vk_types.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <vector>

enum class MeshType {
    CubeBlock,
    QuadScreen,
    QuadBlock,
};

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription>   bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct alignas(16) VertexBlock {
    glm::vec3                     position;
    glm::vec3                     normal;
    glm::vec2                     uv;
    uint32_t                      faceIndex;
    static VertexInputDescription vertex_input_description();
};

struct alignas(16) VertexQuadScreen {
    glm::vec2                     uv;
    static VertexInputDescription vertex_input_description();
};
struct MeshContext {
    size_t          verticeSize;
    AllocatedBuffer verticeBuffer;
    VertexInputDescription (*inputDescription)(void);
};

class Mesh {

  public:
    void        init_mesh(ResourceContext &context);
    MeshContext get_mesh(MeshType type);

  private:
    ResourceIndex                             ci_blueprintIndex;
    std::unordered_map<MeshType, MeshContext> ci_meshes;
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

    void       init_texture();
    GPUTexture get_texture(Block::Type blockType);
    uint32_t   get_vertices_size();

} // namespace Block

struct PNGContext {
    size_t   texWidth, texHeight, pixelSize;
    uint8_t *pixels;
};

PNGContext load_png(const char *filePath);
