#pragma once

#include "vk_types.h"
#include <vulkan/vulkan_core.h>

namespace vkutil {

    extern PFN_vkSetDebugUtilsObjectNameEXT cmdSetDebugUtilsObjectNameEXT;

    VkCommandPool   create_pool(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag);
    VkCommandBuffer create_cmd(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPool pool);
    VkFence         create_fence(VulkanContextGlobal context);
    VkSemaphore     create_semaphore(VulkanContextGlobal context);

    void transition_image_color_attachment(VkCommandBuffer cmd, VkImage image);
    void transition_image_present(VkCommandBuffer cmd, VkImage image);
    void transition_image_layout(VkCommandBuffer cmd, VkImageLayout newLayout, TImage &imageContext);

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandBuffer cmd, VkQueue queue);

    bool load_shader_module(VkDevice device, const char *filePath, VkShaderModule *outShaderModule);

    void debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, VkDevice device);
} // namespace vkutil
