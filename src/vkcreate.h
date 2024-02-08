#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "camera/camera.h"
#include "vk_descriptors.h"
#include "vk_mesh.h"
#include "vk_types.h"

// create more speciic ones later

struct DescriptorSetBuffer {
  AllocatedBuffer buffer;
  VkDescriptorSet set;
};

struct DescriptorSetImage {
  AllocatedBuffer buffer;
  VkDescriptorSet set;
};
// make a builder for this one
struct DescriptorSets {
  std::vector<VkDescriptorSet> sets;
  std::vector<AllocatedBuffer> allocation;
};

struct PipelineInfo {
  VkPipelineLayout pipeline_layout;
  std::vector<VkDescriptorSetLayout &> desc_layouts;
};

class GlobalState {
 public:
  void add_descriptor_layout(vkutil::DescriptorBuilder builder, const char *key);
  void add_descriptor_set(VkDescriptorSetLayout layout);

  void write_descriptor_set(VkDescriptorSetLayout layout, int index);

  std::unordered_map<const char *, VkDescriptorSetLayout> desc_layouts;

  /// @brief
  std::unordered_map<VkDescriptorSetLayout, DescriptorSets> descriptors;

  /// @brief layouts for the pipeline
  std::unordered_map<VkPipeline, PipelineInfo> pipeline_info;

  vkutil::DescriptorAllocator *_globalAllocator;
  vkutil::DescriptorLayoutCache *_globalLayoutCache;
};
