#pragma once

#include "vk_types.h"

class Helper {
public:
  static void init(VkDevice device, VkPhysicalDeviceProperties gpuProperties,
                   VmaAllocator allocator);

  static size_t pad_uniform_buffer_size(size_t originalSize);

  static AllocatedBuffer *create_buffer(size_t allocSize,
                                        VkBufferUsageFlags usage,
                                        VmaMemoryUsage memoryUsage);
  static bool load_shader_module(const char *filePath,
                                 VkShaderModule *outShaderModule);

  static std::string get_filename_from_path(const char *path);
  // TODO make getters
  static VkDevice device;
  static VkPhysicalDeviceProperties gpuProperties;
  static VmaAllocator allocator;

private:
  /*Handlers*/
};
