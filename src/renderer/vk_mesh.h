#pragma once

#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <vector>

#include "vk_types.h"

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct alignas(16) VertexTemp {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    uint32_t faceIndex;
};

struct alignas(16) VertexQuadScreen {
    glm::vec2 uv;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
    static VertexInputDescription get_vertex_description();
};

struct VertexOpengl {
    glm::vec3 position;
    static VertexInputDescription get_vertex_description();
};

struct TextureAtlas {
    std::unordered_map<std::string, glm::vec2[]> textureAtlas;
};

struct Mesh {
    std::vector<Vertex> _vertices;

    AllocatedBuffer _vertexBuffer;

    bool load_from_obj(const char *filename);
};

struct MeshData {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> index;
};

VertexInputDescription vertex_input_description();
