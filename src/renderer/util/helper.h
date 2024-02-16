#pragma once

#include <vulkan/vulkan_core.h>

#include <functional>

#include "../vk_types.h"

class Helper {
  public:
    static void init(VkDevice device, VkPhysicalDeviceProperties gpuProperties, VmaAllocator allocator, VkCommandBuffer cmd, VkQueue graphic);

    static size_t pad_uniform_buffer_size(size_t originalSize);

    static AllocatedBuffer create_buffer(size_t allocSize,
                                         VkBufferUsageFlags usage,
                                         VmaMemoryUsage memoryUsage);
    static bool load_shader_module(const char* filePath,
                                   VkShaderModule* outShaderModule);
    static void
    immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    void load_image_slices(const char* file,
                           std::vector<AllocatedImage>& outImage, size_t sizeX,
                           size_t sizeY);

    void load_image(const char* file, AllocatedImage& outImage);

    static std::string get_filename_from_path(const char* path);
    // TODO make getters
    static VkDevice device;
    static VkPhysicalDeviceProperties gpuProperties;
    static VmaAllocator allocator;
    static VkCommandBuffer main_cmd;
    static VkQueue graphicQueue;

  private:
};
