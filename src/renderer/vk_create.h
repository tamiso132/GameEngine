#pragma once

#include <stdint.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/vk_descriptors.h"
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

struct DescriptorSet {
    std::vector<std::optional<AllocatedBuffer>> bindingsPointers;
    VkDescriptorSet descriptorSet;
};

enum LayoutPhase {
    PerFrame,
    PerRenderPass,
    PerMaterial,
    PerObject
};

struct PipelineInfo {
    VkPipelineLayout pipeline_layout;
    std::vector<std::reference_wrapper<VkDescriptorSetLayout>> desc_layouts;
    std::vector<size_t> indices;
};

/*GlobalState*/
class GlobalBuilder;

class GlobalState {
    friend GlobalBuilder;

  public:
    void init(VkDevice device);
    GlobalBuilder begin_build_descriptor();
    void write_descriptor_set(const char *layerName, int bindingIndex,
                              VmaAllocator allocator, void *data,
                              size_t dataSize);

    VkDescriptorSetLayout get_descriptor_layout(const char *key);

    DescriptorSet get_descriptor_set(const char *key);

  private:
    // Forward declaration

    // Private member functions
    void add_descriptor_set(VkDescriptorSetLayout layout, VkDescriptorSet set,
                            const char *key,
                            std::vector<std::optional<AllocatedBuffer>> buffers);

    // Private data members
    std::unordered_map<const char *, VkDescriptorSetLayout> descLayout;
    std::unordered_map<VkDescriptorSetLayout, DescriptorSet> descSet;
    std::unordered_map<VkPipeline, PipelineInfo> pipInfo;

    vkutil::DescriptorAllocator *_globalAllocator;
    vkutil::DescriptorLayoutCache *_globalLayoutCache;

    // Nested struct
};

class GlobalBuilder {
    friend GlobalState;

  public:
    GlobalBuilder(vkutil::DescriptorLayoutCache *layoutCache,
                  vkutil::DescriptorAllocator *allocator,
                  GlobalState &globalState)
        : refState(globalState) {
        this->builder = this->builder.begin(layoutCache, allocator);
    }
    GlobalBuilder &bind_create_buffer(size_t buffer_max_size,
                                      BufferType usage_type,
                                      VkShaderStageFlags stageFlags);

    GlobalBuilder *bind_image(VkDescriptorImageInfo *imageInfo, ImageType type,
                              VkShaderStageFlags stageFlags);

    bool build(const char *key);

  private:
    vkutil::DescriptorBuilder builder;
    size_t currentBinding;
    std::vector<std::optional<AllocatedBuffer>> allocBuffers;

    std::vector<VkDescriptorBufferInfo> bufferInfos;

    GlobalState &refState;
};
