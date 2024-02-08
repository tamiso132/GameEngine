#include "vkcreate.h"

#include "helper.h"

void GlobalInformation::add_layout(vkutil::DescriptorBuilder builder) {
  VkDescriptorSet set;
  VkDescriptorSetLayout layout;
  builder.build(set, layout);

  if (!this->descriptors.contains(layout)) {
    DescriptorSets sets;
    sets.sets.push_back(set);
    this->descriptors.insert({layout, sets});
  } else {
    this->descriptors[layout].sets.push_back(set);
  }
};
