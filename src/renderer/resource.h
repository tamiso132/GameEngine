#include "descriptor.h"
#include "util/helper.h"
#include "util/vk_initializers.h"
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

struct ImageAlloc {
    VkSampler      sampler;
    VkImageView    view;
    AllocatedImage alloc;
    uint8_t        extentIndex;
};

struct Resource {
    std::unordered_map<BufferHandle, AllocatedBuffer> ci_buffers;
    std::unordered_map<ImageHandle, ImageAlloc>       ci_images;
    VmaAllocator                                      ci_allocator;
    std::vector<VkExtent3D>                           ci_extents;

    uint32_t ci_counterBuffer = 0;
    uint32_t ci_counterImage  = 0;

    VkQueue  ci_transferQueue;
    VkDevice ci_device;

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance) {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = physical;
        allocatorInfo.device                 = device;
        allocatorInfo.instance               = instance;

        ci_device = device;

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

        vmaCreateBuffer(ci_allocator, &bufferInfo, &allocInfo, &ci_buffers[ci_counterBuffer]._buffer, &ci_buffers[ci_counterBuffer]._allocation, nullptr);

        ci_counterBuffer++;
        return ci_counterBuffer - 1;
    }

    ImageHandle create_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, VmaMemoryUsage memoryUsage, VkMemoryPropertyFlagBits memoryFlag,
                                       VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT, VkSampler _Nullable *sampler = nullptr) {
        auto imageInfo = vkinit::image_create_info(format, usageFlags, extent);

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage                   = memoryUsage;
        allocInfo.requiredFlags           = memoryFlag;

        VK_CHECK(vmaCreateImage(ci_allocator, &imageInfo, &allocInfo, &ci_images[ci_counterImage].alloc._image, &ci_images[ci_counterImage].alloc._allocation, nullptr));

        auto imageView = vkinit::imageview_create_info(format, ci_images[ci_counterImage].alloc._image, aspectFlag);

        VK_CHECK(vkCreateImageView(ci_device, &imageView, nullptr, &ci_images[ci_counterImage].view));

        if (sampler == nullptr) {
            auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
            vkCreateSampler(ci_device, &samplerInfo, nullptr, &ci_images[ci_counterImage].sampler);
        } else {
            ci_images[ci_counterImage].sampler = *sampler;
        }

        ci_counterImage++;
        return ci_counterImage - 1;
    }

    void write_to_image(bool immediate, VkCommandBuffer cmd, ImageHandle imageSrc, ImageHandle imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout) {
        VkImageCopy imageRegion;

        VkImageSubresourceLayers srcSub;
        VkImageSubresourceLayers dstSub;

        srcSub.baseArrayLayer = 0;
        srcSub.mipLevel       = 0;
        srcSub.layerCount     = 1;

        dstSub.baseArrayLayer = 0;
        dstSub.mipLevel       = 0;
        dstSub.layerCount     = 1;

        imageRegion.srcOffset = {0, 0, 0};
        imageRegion.dstOffset = {0, 0, 0};
        imageRegion.extent    = ci_extents[imageSrc];

        imageRegion.srcSubresource = srcSub;
        imageRegion.dstSubresource = dstSub;

        if (immediate) {
            VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            vkBeginCommandBuffer(cmd, &beginInfo);
            vkCmdCopyImage(cmd, ci_images[imageSrc].alloc._image, srcLayout, ci_images[imageDst].alloc._image, dstLayout, 1, &imageRegion);

            VkSubmitInfo submit = vkinit::submit_info(&cmd);

            // submit command buffer to the queue and execute it.
            VK_CHECK(vkQueueSubmit(ci_transferQueue, 1, &submit, nullptr));

            vkQueueWaitIdle(ci_transferQueue);
        }
    }

    void write_to_image(bool isHost, VkCommandBuffer cmd, ImageHandle imageSrc, void *src, size_t sizeOfData) {

        void *dst;
        if (isHost) {
            vmaMapMemory(ci_allocator, ci_images[imageSrc].alloc._allocation, &dst);
            memcpy(dst, src, sizeOfData);
            vmaUnmapMemory(ci_allocator, ci_images[imageSrc].alloc._allocation);
        } else {
            
        }
    }

    void create_image_texture_array() {}
};