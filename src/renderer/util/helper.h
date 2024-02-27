#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <functional>

#include "../vk_types.h"

class Helper {
  public:
    static void init(VkDevice device, VkPhysicalDeviceProperties gpuProperties, VmaAllocator allocator, VkCommandBuffer cmd, VkQueue graphic);

    static size_t pad_uniform_buffer_size(size_t originalSize);

    static AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    static bool load_shader_module(const char *filePath, VkShaderModule *outShaderModule);
    static void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);

    static void transition_image_layout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, VkCommandBuffer cmd = Helper::main_cmd);

    void load_image_slices(const char *file, std::vector<AllocatedImage> &outImage, size_t sizeX, size_t sizeY);

    static void create_image(VkExtent3D extent, VkFormat format, VkImageAspectFlags vkImageAspectFlag, AllocatedImage *image, VkImageView *view);
    void static load_image(const char *file, AllocatedImage &outImage);

    void load_test_image(AllocatedImage &outImage);

    static void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    static std::string get_filename_from_path(const char *path);
    // TODO make getters
    static VkDevice device;
    static VkPhysicalDeviceProperties gpuProperties;
    static VmaAllocator allocator;
    static VkCommandBuffer main_cmd;
    static VkQueue graphicQueue;
    static void create_cube_map(const char *fileAtlas, uint32_t gridLength, std::vector<std::pair<uint32_t, uint32_t>> cubeMapping, AllocatedImage *cubeMap);
    static void create_texture_array(const char *fileAtlas, uint32_t gridLength, AllocatedImage &imageArray, uint32_t &layers);

  private:
};
