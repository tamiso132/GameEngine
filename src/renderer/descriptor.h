#pragma once

#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

struct DescriptorState {
    std::unordered_map<int32_t, AllocatedBuffer> allocatedBuffer;
    std::unordered_map<int32_t, AllocatedImage>  allocatedImage;
    VkDescriptorSet                              set;
    VkDescriptorSetLayout                        layout;
};

struct Desc {
    std::unordered_map<std::string, DescriptorState> descriptorStates;
};

//> descriptor_layout
struct DescriptorLayoutBuilder {

    void add_binding(uint32_t binding, VkDescriptorType type);
    void add_buffer(size_t bufferSize, VkBufferUsageFlags bufferType);
    void add_image(VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void clear();

    // VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

  private:
    std::unordered_map<uint32_t, AllocatedBuffer> buffers;
    std::unordered_map<uint32_t, AllocatedImage>  allocatedImage;
    uint32_t                                      binding;
    std::vector<VkDescriptorSetLayoutBinding>     bindings;
};
//< descriptor_layout
//
//> writer
struct DescriptorWriter {
    std::deque<VkDescriptorImageInfo>  imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet>  writes;

    void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void clear();
    void update_set(VkDevice device, VkDescriptorSet set);
};
//< writer

//< descriptor_allocator

//> descriptor_allocator_grow
struct DescriptorAllocatorGrowable {
  public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float            ratio;
    };

    void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void clear_pools(VkDevice device);
    void destroy_pools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

  private:
    VkDescriptorPool get_pool(VkDevice device);
    VkDescriptorPool create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio>    ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t                      setsPerPool;
};
//< descriptor_allocator_grow