#pragma once

#include "vk_types.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
struct VertexInputDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

  VkPipelineVertexInputStateCreateFlags flags = 0;
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

struct Mesh {
  std::vector<Vertex> _vertices;

  AllocatedBuffer _vertexBuffer;

  bool load_from_obj(const char *filename);
};

void vertex_input_description(
    VkPipelineVertexInputStateCreateInfo &vertexInputState);