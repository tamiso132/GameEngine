#include "vkcreate.h"

#include "helper.h"

void GlobalState::add_descriptor_layout(vkutil::DescriptorBuilder builder, const char* key) {
  VkDescriptorSet set;
  VkDescriptorSetLayout layout;
  builder.build(set, layout);
  assert(!this->descriptors.contains(layout));
  this->descriptors[layout].sets.push_back(set);
}
void GlobalState::add_descriptor_set(VkDescriptorSetLayout layout) { vkutil::DescriptorBuilder builder; };

void GlobalState::write_descriptor_set(VkDescriptorSetLayout layout, int index) {
  VkDescriptorSet set_to_write = this->descriptors[layout].sets[index];
}
