#pragma once

#include "VkBootstrapDispatch.h"
#include "mesh.h"

namespace Render {
    void init(VulkanContextGlobal &vulkanContext, VmaAllocator &allocator, vkb::DispatchTable &table, ResourceContext &resourceContext, Mesh &meshes);
    void prepare_frame(VulkanContextGlobal vulkanContext, VkCommandBuffer cmd, VkFence &queueIsDone, VkSemaphore imageIsAquired, uint32_t &imageIndex);
    void end_frame(const VulkanContextGlobal &vulkanContext, VkCommandBuffer cmd, VkSemaphore &renderIsDone, VkFence &queueIsDone, VkSemaphore &imageIsAquired, uint32_t &imageIndex);
} // namespace Render