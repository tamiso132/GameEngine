#include "vk_create.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#include "util/helper.h"
#include "vk_create.h"
#include "vk_types.h"

/*Global State*/

void GlobalState::init(VkDevice device) {
    this->_globalAllocator = new vkutil::DescriptorAllocator{};
    this->_globalLayoutCache = new vkutil::DescriptorLayoutCache{};

    this->_globalAllocator->init(device);
    this->_globalLayoutCache->init(device);
}

GlobalBuilder GlobalState::begin_build_descriptor() {

    GlobalBuilder global_builder = GlobalBuilder(this->_globalLayoutCache, this->_globalAllocator, *this);
    global_builder.currentBinding = 0;
    return global_builder;
}

VkDescriptorSetLayout GlobalState::get_descriptor_layout(const char *key) {
    int x = 5;
    // assert(this->descLayout.contains(key) == false);

    return this->descLayout[key];
}

DescriptorSet GlobalState::get_descriptor_set(const char *key) { return this->descSet[this->descLayout[key]]; }

void GlobalState::add_descriptor_set(VkDescriptorSetLayout layout, VkDescriptorSet set, const char *key, std::vector<std::optional<AllocatedBuffer>> buffers) {

    assert(this->descLayout.find(key) == this->descLayout.end());
    assert(this->descSet.find(layout) == this->descSet.end());

    DescriptorSet desc_set;
    desc_set.descriptorSet = set;
    desc_set.bindingsPointers = buffers;

    this->descLayout.insert({key, layout});
    this->descSet.insert({layout, desc_set});
}

void GlobalState::write_descriptor_set(const char *layerName, int bindingIndex, VmaAllocator allocator, void *srcData, size_t srcSize) {
    DescriptorSet set = this->descSet[this->descLayout[layerName]];
    void *dstData;
    vmaMapMemory(allocator, set.bindingsPointers[bindingIndex].value()._allocation, &dstData);

    std::memcpy(dstData, srcData, srcSize);

    vmaUnmapMemory(allocator, set.bindingsPointers[bindingIndex].value()._allocation);
}

/*Global Builder*/

GlobalBuilder &GlobalBuilder::bind_create_buffer(size_t buffer_max_size, BufferType usage_type, VkShaderStageFlags stageFlags) {
    VkDescriptorBufferInfo info;
    VkBufferUsageFlags buffer_type;

    switch (usage_type) {
    case BufferType::DYNAMIC_STORAGE:
    case BufferType::STORAGE:
        buffer_type = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        break;
    default:
        buffer_type = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        break;
    }
    this->allocBuffers.push_back(Helper::create_buffer(buffer_max_size, buffer_type, VMA_MEMORY_USAGE_CPU_TO_GPU));

    info.offset = 0;
    info.range = buffer_max_size;
    info.buffer = this->allocBuffers[this->allocBuffers.size() - 1].value()._buffer;

    bufferInfos.push_back(info);

    this->builder = this->builder.bind_buffer(this->currentBinding, &bufferInfos[bufferInfos.size() - 1], (VkDescriptorType)usage_type, stageFlags);

    this->currentBinding++;
    return *this;
}
/// @brief Non dynamic image, cannot be updated
GlobalBuilder *GlobalBuilder::bind_image(VkDescriptorImageInfo *imageInfo, ImageType type, VkShaderStageFlags stageFlags) {
    this->builder.bind_image(this->currentBinding, imageInfo, (VkDescriptorType)type, stageFlags);
    this->allocBuffers.push_back(std::nullopt);
    this->currentBinding++;
    return this;
}

bool GlobalBuilder::build(const char *key) {
    VkDescriptorSet set;
    VkDescriptorSetLayout layout;

    this->builder.build(set, layout);
    this->refState.add_descriptor_set(layout, set, key, this->allocBuffers);

    // cleanup this

    // placeholder
    return true;
}
