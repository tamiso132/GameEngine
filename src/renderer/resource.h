#pragma once

#include "descriptor.h"
#include "util/helper.h"
#include "util/vk_initializers.h"
#include "vk_mem_alloc.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef uint32_t BufferHandle;
typedef uint32_t ImageHandle;

namespace vkinit {
    VkBufferImageCopy buffer_image_copy_info(VkExtent3D extent);
    VkImageCopy       image_image_copy_info(VkExtent3D extent);
} // namespace vkinit

struct ResourceBuilderContext {};

//> descriptor_layout
struct DescriptorLayoutBuilder {

    void add_binding(uint32_t binding, VkDescriptorType type);
    void add_buffer(BufferHandle handle);
    void add_image(ImageHandle handle);
    void clear();

    struct Context {

        uint32_t                                   bindingCount;
        std::vector<VkDescriptorSetLayoutBinding>  bindings;
        std::unordered_map<uint32_t, BufferHandle> buffersBindings;
        std::unordered_map<uint32_t, ImageHandle>  imageBindings;
    };

    // VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    Context get_context();

  private:
    Context ci_context;
};

struct ResourceBuilder {
    struct Blueprint {
        
    };

    void create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlagBits memoryFlag);
    void create_buffer_gpu_only(size_t allocSize, VkBufferUsageFlags usage);
    void create_buffer_visible(size_t allocSize, VkBufferUsageFlags usage);
    void create_staging_buffer(size_t allocSize, VkMemoryPropertyFlagBits memoryFlag);
    void create_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlagBits memoryFlag,
                                VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT, VkSampler _Nullable *sampler = nullptr);

    Blueprint get_blueprint();
};

struct ImageContext {
    VkSampler      sampler;
    VkImageView    view;
    AllocatedImage alloc;
    uint8_t        extentIndex;
};

struct BufferContext {
    AllocatedBuffer    alloc;
    VkDescriptorType   type;
    VkBufferUsageFlags usage;
};

struct ResourceContext {
    std::unordered_map<BufferHandle, AllocatedBuffer> ci_buffers;
    std::unordered_map<ImageHandle, ImageContext>     ci_images;
    VmaAllocator                                      ci_allocator;
    std::vector<VkExtent3D>                           ci_extents;

    VkQueue  ci_transferQueue;
    uint32_t ci_transferQueueFamily;

    VkDevice ci_device;

    VkCommandBuffer ci_cmd;
    VkCommandPool   ci_pool;

    uint32_t ci_bufferCounter;
    uint32_t ci_imageCounter;

    DescriptorAllocatorGrowable ci_descriptorAllocator;

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VkQueue transferQueue, uint32_t transferQueueFamily) {

        ci_device              = device;
        ci_transferQueueFamily = transferQueueFamily;
        ci_transferQueue       = transferQueue;

        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(ci_transferQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &ci_pool));

        VkCommandBufferAllocateInfo bufferInfo = vkinit::command_buffer_allocate_info(ci_pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(device, &bufferInfo, &ci_cmd));

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = physical;
        allocatorInfo.device                 = device;
        allocatorInfo.instance               = instance;

        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &ci_allocator));
    };

    BufferHandle create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlagBits memoryFlag) {

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.usage              = usage;
        bufferInfo.size               = allocSize;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = memoryUsage;
        allocInfo.requiredFlags           = memoryFlag;

        vmaCreateBuffer(ci_allocator, &bufferInfo, &allocInfo, &ci_buffers[ci_bufferCounter]._buffer, &ci_buffers[ci_bufferCounter]._allocation, nullptr);

        ci_bufferCounter++;
        return ci_bufferCounter - 1;
    }

    void copy_to_buffer_visible(BufferHandle dst, size_t size, void *src) {
        void *dstV;
        vmaMapMemory(ci_allocator, ci_buffers[dst]._allocation, &dstV);
        memcpy(dstV, src, size);
        vmaUnmapMemory(ci_allocator, ci_buffers[dst]._allocation);
    }

    void destroy_buffer(BufferHandle bufferHandle) {
        vmaDestroyBuffer(ci_allocator, ci_buffers[bufferHandle]._buffer, ci_buffers[bufferHandle]._allocation);
        ci_buffers.erase(bufferHandle);
    }

    ImageHandle create_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlagBits memoryFlag,
                                       VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT, VkSampler _Nullable *sampler = nullptr) {
        auto imageInfo = vkinit::image_create_info(format, usageFlags, extent);

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = memoryUsage;
        allocInfo.requiredFlags           = memoryFlag;

        auto imageContext = &ci_images[ci_imageCounter];

        VK_CHECK(vmaCreateImage(ci_allocator, &imageInfo, &allocInfo, &imageContext->alloc._image, &imageContext->alloc._allocation, nullptr));

        auto imageView = vkinit::imageview_create_info(format, imageContext->alloc._image, aspectFlag);

        VK_CHECK(vkCreateImageView(ci_device, &imageView, nullptr, &imageContext->view));

        if (sampler == nullptr) {
            auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
            vkCreateSampler(ci_device, &samplerInfo, nullptr, &imageContext->sampler);
        } else {
            imageContext->sampler = *sampler;
        }

        ci_imageCounter++;
        return ci_imageCounter - 1;
    }

    void write_to_image(ImageHandle imageSrc, ImageHandle imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd = nullptr) {

        auto imageRegion = vkinit::image_image_copy_info(ci_extents[ci_images[imageSrc].extentIndex]);

        if (cmd == nullptr) {

            VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            vkBeginCommandBuffer(ci_cmd, &beginInfo);

            VkSubmitInfo submit = vkinit::submit_info(&ci_cmd);
            VK_CHECK(vkQueueSubmit(ci_transferQueue, 1, &submit, nullptr));

            vkQueueWaitIdle(ci_transferQueue);

        } else {
            vkCmdCopyImage(cmd, ci_images[imageSrc].alloc._image, srcLayout, ci_images[imageDst].alloc._image, dstLayout, 1, &imageRegion);
        }
    }

    void write_to_image_from_cpu(ImageHandle imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd = nullptr) {

        bool               isImmediate;
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.usage;
        BufferHandle stagingBuffer = create_buffer(sizeOfData, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        copy_to_buffer_visible(stagingBuffer, sizeOfData, src);
        auto beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        if (cmd == nullptr) {
            isImmediate = true;
            vkBeginCommandBuffer(cmd, &beginInfo);
            cmd = ci_cmd;
        }

        auto imageDst = ci_images[imageDstHandle];

        transition_image_layout(cmd, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageDstHandle);
        auto copyRegion = vkinit::buffer_image_copy_info(ci_extents[ci_images[imageDstHandle].extentIndex]);

        vkCmdCopyBufferToImage(cmd, ci_buffers[stagingBuffer]._buffer, imageDst.alloc._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        transition_image_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout, imageDstHandle);

        if (isImmediate) {
            // send it
            vkEndCommandBuffer(cmd);
            auto submitInfo = vkinit::submit_info(&cmd);
            VK_CHECK(vkQueueSubmit(ci_transferQueue, 1, &submitInfo, nullptr));

            vkQueueWaitIdle(ci_transferQueue);
        }
    }

    void transition_image_layout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout, ImageHandle image) {
        const VkImageMemoryBarrier image_memory_barrier2{.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                         .srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                         .oldLayout        = oldLayout,
                                                         .newLayout        = newLayout,
                                                         .image            = ci_images[image].alloc._image,
                                                         .subresourceRange = {
                                                             .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                             .baseMipLevel   = 0,
                                                             .levelCount     = 1,
                                                             .baseArrayLayer = 0,
                                                             .layerCount     = 1,
                                                         }};

        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
                             0, 0, nullptr, 0, nullptr,
                             1,                     // imageMemoryBarrierCount
                             &image_memory_barrier2 // pImageMemoryBarriers
        );
    }

    void build_descriptor_layout(DescriptorLayoutBuilder::Context builderContext) {}
};
