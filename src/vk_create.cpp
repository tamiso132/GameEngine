#include "vk_create.h"

#include <cassert>
#include <cstring>
#include <string>
#include <unordered_map>

#include "helper.h"
#include "vk_create.h"

/*Global State*/

void GlobalState::init(VkDevice device) {
  this->_globalAllocator = new vkutil::DescriptorAllocator{};
  this->_globalLayoutCache = new vkutil::DescriptorLayoutCache{};
  
  this->_globalAllocator->init(device);
  this->_globalLayoutCache->init(device);
}

GlobalBuilder GlobalState::begin_build_descriptor() {

  auto global_builder = GlobalBuilder(*this);
  global_builder.builder.begin(this->_globalLayoutCache,
                               this->_globalAllocator);
  return global_builder;
}

VkDescriptorSetLayout GlobalState::get_descriptor_layout(const char *key) {
  assert(!this->descLayout.contains(key));

  return this->descLayout[key];
}

DescriptorSet GlobalState::get_descriptor_set(const char *key) {
  return this->descSet[this->descLayout[key]];
}

void GlobalState::add_descriptor_set(
    VkDescriptorSetLayout layout, VkDescriptorSet set, const char *key,
    std::vector<std::optional<AllocatedBuffer>> buffers) {

  std::unordered_map<std::string, int> test;
  assert(this->descLayout.find(key) == this->descLayout.end());
  assert(this->descSet.find(layout) == this->descSet.end());

  DescriptorSet desc_set;
  desc_set.descriptorSet = set;
  desc_set.bindingsPointers = buffers;

  this->descLayout.insert({key, layout});
  this->descSet.insert({layout, desc_set});
}

void GlobalState::write_descriptor_set(const char *layerName, int bindingIndex,
                                       VmaAllocator allocator, void *srcData,
                                       size_t srcSize) {
  DescriptorSet set = this->descSet[this->descLayout[layerName]];
  void *dstData;
  vmaMapMemory(allocator,
               set.bindingsPointers[bindingIndex].value()._allocation,
               &dstData);

  std::memcpy(dstData, srcData, srcSize);

  vmaUnmapMemory(allocator,
                 set.bindingsPointers[bindingIndex].value()._allocation);
  //   vmaMapMemory(_allocator, get_current_frame().cameraBuffer._allocation,
  //   &data);

  //   memcpy(data, &camData, sizeof(GPUCameraData));

  //   vmaUnmapMemory(_allocator, get_current_frame().cameraBuffer._allocation);
}

/*Global Builder*/
GlobalBuilder &
GlobalBuilder::bind_create_buffer(size_t buffer_max_size, BufferType usage_type,
                                  VkShaderStageFlags stageFlags) {
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

  AllocatedBuffer buffer = Helper::create_buffer(buffer_max_size, buffer_type,
                                                 VMA_MEMORY_USAGE_CPU_TO_GPU);
  info.offset = 0;
  info.range = buffer_max_size;
  info.buffer = buffer._buffer;

  this->builder.bind_buffer(this->currentBinding, &info,
                            (VkDescriptorType)usage_type, stageFlags);

  this->allocBuffers.push_back(std::optional(buffer));
  return *this;
}
/// @brief Non dynamic image, cannot be updated
GlobalBuilder &GlobalBuilder::bind_image(VkDescriptorImageInfo *imageInfo,
                                         ImageType type,
                                         VkShaderStageFlags stageFlags) {
  this->builder.bind_image(this->currentBinding, imageInfo,
                           (VkDescriptorType)type, stageFlags);
  this->allocBuffers.push_back(std::nullopt);
  return *this;
}

bool GlobalBuilder::build(const char *key) {
  VkDescriptorSet set;
  VkDescriptorSetLayout layout;

  this->builder.build(set, layout);
  this->global.add_descriptor_set(layout, set, key, this->allocBuffers);

  // placeholder
  return true;
}
