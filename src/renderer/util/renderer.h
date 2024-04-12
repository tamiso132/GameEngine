#pragma once

#include "VkBootstrapDispatch.h"
#include "mesh.h"
#include <any>
#include <memory>
#include <vector>

namespace Render {

    // BASIC RENDER
    void init(VulkanContextGlobal &vulkanContext, VmaAllocator &allocator, vkb::DispatchTable &table, ResourceContext &resourceContext, Mesh &meshes, void *newFeatures = nullptr);
    void prepare_frame(VulkanContextGlobal vulkanContext, VkCommandBuffer cmd, VkFence &queueIsDone, VkSemaphore imageIsAquired, uint32_t &imageIndex);
    void end_frame(const VulkanContextGlobal &vulkanContext, VkCommandBuffer cmd, VkSemaphore &renderIsDone, VkFence &queueIsDone, VkSemaphore &imageIsAquired, uint32_t &imageIndex);

    // IMGUI
    void add_imgui(VulkanContextGlobal &vulkanContext, VkDescriptorPool &imguiDescriptorPool);
    void prepare_frame_imgui(const VulkanContextGlobal &vulkanContext);
    void end_frame_imgui();
} // namespace Render
