#pragma once

#include <stdint.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "vk_descriptors.h"
#include "vk_types.h"

enum BufferType {
  UNIFORM = 6,
  STORAGE = 7,
  DYNAMIC_UNIFORM = 8,
  DYNAMIC_STORAGE = 9,
};

enum ImageType {
  SAMPLER = 0,
  COMBINED_IMAGE_SAMPLER = 1,
  SAMPLED_IMAGE = 2,
  STORAGE_IMAGE = 3,
};

struct PipelineInfo {
  VkPipelineLayout pipeline_layout;
  std::vector<std::reference_wrapper<VkDescriptorSetLayout>> desc_layouts;
  std::vector<size_t> indices;
};

/*GlobalState*/
class GlobalBuilder;

class GlobalState {
  friend class GlobalBuilder;
  struct DescriptorSet {
    /// Allocated buffer element can be null, if element is not dynamic.
    std::vector<std::optional<AllocatedBuffer>> bindingsPointers;
    VkDescriptorSet descriptorSet;
  };

 public:
  GlobalBuilder begin_build_descriptor();

  void write_descriptor_set(VkDescriptorSetLayout layout, int descriptor_index, int binding_index);

 private:
  void add_descriptor_set(VkDescriptorSetLayout layout, VkDescriptorSet set, const char *key,
                          std::vector<std::optional<AllocatedBuffer>> buffers);

  std::unordered_map<const char *, VkDescriptorSetLayout> descLayout;

  /// @brief
  std::unordered_map<VkDescriptorSetLayout, DescriptorSet> descSet;

  /// @brief layouts for the pipeline
  std::unordered_map<VkPipeline, PipelineInfo> pipInfo;

  vkutil::DescriptorAllocator *_globalAllocator;
  vkutil::DescriptorLayoutCache *_globalLayoutCache;
};

class GlobalBuilder {
  enum BufferUsage {
    UNIFORM = 0x00000010,
    STORAGE = 0x00000020,
  };

  friend class GlobalState;  // Declare GlobalState as a friend class

  GlobalBuilder &bind_create_buffer(size_t buffer_max_size, BufferType usage_type, VkShaderStageFlags stageFlags);

  GlobalBuilder &bind_image(VkDescriptorImageInfo *imageInfo, ImageType type, VkShaderStageFlags stageFlags);

  bool build(const char *key);

 private:
  GlobalBuilder(GlobalState &globalState) : global(globalState) {}

  vkutil::DescriptorBuilder builder;
  size_t currentBinding;
  GlobalState &global;

  std::vector<std::optional<AllocatedBuffer>> allocBuffers;
};
