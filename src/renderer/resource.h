#pragma once

#include "VkBootstrapDispatch.h"
#include "descriptor.h"
#include "util/helper.h"
#include "util/vk_initializers.h"
#include "vk_mem_alloc.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef VkResult(VKAPI_PTR *PFN_vkSetDebugUtilsObjectNameEXT)(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo);
typedef uint32_t ResourceIndex;

namespace vkinit {
    VkBufferImageCopy buffer_image_copy_info(VkExtent3D extent);
    VkImageCopy       image_image_copy_info(VkExtent3D extent);
    void              debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, PFN_vkSetDebugUtilsObjectNameEXT setDebugName, VkDevice device);
} // namespace vkinit

enum class BufferType {
    Staging = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    Vertex  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    Index   = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
};

enum class MemoryType {
    BestFit,
    GpuOnly     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    HostVisible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
};

struct ImageContext {
    VkSampler      sampler;
    VkImageView    view;
    AllocatedImage alloc;
    uint8_t        extentIndex;
};

struct BufferContext {
    AllocatedBuffer    alloc;
    VkBufferUsageFlags usage;
};

//> descriptor_layout
struct DescriptorLayoutBuilder {

    void add_binding(uint32_t binding, VkDescriptorType type);
    void add_buffer(ResourceIndex handle);
    void add_image(ResourceIndex handle, VkImageLayout layout);
    void clear();

    struct Blueprint {

        uint32_t                                    bindingCount;
        std::vector<VkDescriptorSetLayoutBinding>   bindings;
        std::unordered_map<uint32_t, ResourceIndex> bufferIndices;
        std::unordered_map<uint32_t, ResourceIndex> imageIndices;
        VkShaderStageFlags                          shaderStage;
        std::string                                 name;
    };

    // VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    Blueprint get_blueprint();

  private:
    Blueprint ci_context;
};

struct ResourceBuilder {
    struct BufferObjectCreateInfo {
        VkBufferCreateInfo      bufferInfo;
        VmaAllocationCreateInfo allocInfo;
        std::string             name;
    };

    struct ImageObjectCreateInfo {
        VmaAllocationCreateInfo allocInfo;
        VkImageCreateInfo       imageInfo;
        VkImageViewCreateInfo   imageViewInfo;
        VkSampler               sampler;
        std::string             name;
    };
    struct Blueprint {
        std::vector<BufferObjectCreateInfo> bufferInfos;
        std::vector<ImageObjectCreateInfo>  imageInfos;
    };

    void add_buffer(size_t allocSize, BufferType type, const char *objectName, MemoryType memory = MemoryType::BestFit) {
        VkMemoryPropertyFlags memoryType;

        if (memory == MemoryType::HostVisible || memory == MemoryType::GpuOnly) {
            memoryType = (VkMemoryPropertyFlags)memory;
        } else {
            memoryType = (VkMemoryPropertyFlags)MemoryType::HostVisible;

            switch (type) {
            case BufferType::Vertex:
            case BufferType::Index:
                memoryType = (VkMemoryPropertyFlags)MemoryType::GpuOnly;
                break;
            }
        }

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.usage              = (VkBufferUsageFlags)type;
        bufferInfo.size               = allocSize;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.requiredFlags           = memoryType;

        BufferObjectCreateInfo info;
        info.allocInfo  = allocInfo;
        info.bufferInfo = bufferInfo;
        info.name       = std::string(objectName);
    };

    void add_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, const char *objectName, VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT,
                             VkSampler _Nullable sampler = nullptr) {

        auto imageInfo = vkinit::image_create_info(format, usageFlags, extent);

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.requiredFlags           = (VkMemoryPropertyFlags)MemoryType::GpuOnly;

        auto imageView = vkinit::imageview_create_info(format, nullptr, aspectFlag);

        ImageObjectCreateInfo info;

        info.imageInfo     = imageInfo;
        info.allocInfo     = allocInfo;
        info.imageViewInfo = imageView;
        info.sampler       = sampler;
        info.name          = std::string(objectName);

        ci_blueprint.imageInfos.push_back(info);
    };

    Blueprint get_blueprint();

    Blueprint ci_blueprint;
};

std::string get_valid_blueprint_counter_name(ResourceBuilder::Blueprint resourceContext, std::unordered_map<std::string, ResourceIndex> resource_index) {
    std::string extName = "";
    std::string nameCheck;
    if (resourceContext.bufferInfos.size() > 0) {
        nameCheck = resourceContext.bufferInfos[0].name;
    } else {
        nameCheck = resourceContext.imageInfos[0].name;
    }

    int index = 1;
    while (resource_index.contains(nameCheck + extName)) {
        extName = std::to_string(index);
    }

    return extName;
}

struct ResourceContext {
    std::unordered_map<std::string, ResourceIndex>   ci_resourceIndex;
    std::unordered_map<ResourceIndex, BufferContext> ci_buffers;
    std::unordered_map<ResourceIndex, ImageContext>  ci_images;
    VmaAllocator                                     ci_allocator;
    std::vector<VkExtent3D>                          ci_extents;
    PFN_vkSetDebugUtilsObjectNameEXT                 vkSetDebugUtilsObjectNameEXT;
    VkQueue                                          ci_transferQueue;
    uint32_t                                         ci_transferQueueFamily;

    VkDevice ci_device;

    VkCommandBuffer ci_cmd;
    VkCommandPool   ci_pool;
    uint32_t        ci_resourceCounter;

    DescriptorAllocatorGrowable ci_descriptorAllocator;

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VkQueue transferQueue, uint32_t transferQueueFamily, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil) {

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

        this->vkSetDebugUtilsObjectNameEXT = vkSetDebugUtil;

        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &ci_allocator));
    };

    void copy_to_buffer_visible(ResourceIndex dst, size_t size, void *src) {
        void *dstV;
        vmaMapMemory(ci_allocator, ci_buffers[dst]._allocation, &dstV);
        memcpy(dstV, src, size);
        vmaUnmapMemory(ci_allocator, ci_buffers[dst]._allocation);
    }

    void destroy_buffer(ResourceIndex bufferHandle) {
        vmaDestroyBuffer(ci_allocator, ci_buffers[bufferHandle]._buffer, ci_buffers[bufferHandle]._allocation);
        ci_buffers.erase(bufferHandle);
    }

    void write_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd = nullptr) {

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

    void write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd = nullptr) {

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

    void transition_image_layout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout, ResourceIndex image) {
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

    void build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext) {

        for (int i = 0; i < builderContext.bindings.size(); i++) {
            auto binding = builderContext.bindings[i];
            if (builderContext.imageIndices.contains(i)) {
                auto                   imageContext = ci_images[builderContext.imageIndices[i]];
                VkDescriptorImageInfo &info         = VkDescriptorImageInfo{.sampler = imageContext.sampler, .imageView = imageContext.view, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};

                VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

                write.dstBinding      = binding;
                write.dstSet          = VK_NULL_HANDLE; // left empty for now until we need to write it
                write.descriptorCount = 1;
                write.descriptorType  = type;
                write.pImageInfo      = &info;

                writes.push_back(write);
            }
        }

        b.stageFlags |= builderContext.shaderStage;

        VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        info.pNext                           = pNext;

        info.pBindings    = builderContext.bindings.data();
        info.bindingCount = (uint32_t)builderContext.bindings.size();
        info.flags        = flags;

        VkDescriptorSetLayout set;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    }

    void build_resource_blueprint(ResourceBuilder::Blueprint resourceContext) {
        std::string counterTXT = get_valid_blueprint_counter_name(resourceContext, ci_resourceIndex);

        for (auto &bufferInfo : resourceContext.bufferInfos) {

            BufferContext context;
            context.usage = bufferInfo.bufferInfo.usage;

            std::string name = bufferInfo.name + counterTXT;
            vmaCreateBuffer(ci_allocator, &bufferInfo.bufferInfo, &bufferInfo.allocInfo, &context.alloc._buffer, &context.alloc._allocation, nullptr);

            vkinit::debug_object_set_name((uint64_t)ci_buffers[ci_resourceCounter].alloc._buffer, VK_OBJECT_TYPE_BUFFER, name.c_str(), this->vkSetDebugUtilsObjectNameEXT, ci_device);

            ci_resourceIndex[name]         = ci_resourceCounter;
            ci_buffers[ci_resourceCounter] = context;

            ci_resourceCounter++;
        }

        for (auto &imageInfo : resourceContext.imageInfos) {

            ImageContext context;
            std::string  name = imageInfo.name + counterTXT;

            VK_CHECK(vmaCreateImage(ci_allocator, &imageInfo.imageInfo, &imageInfo.allocInfo, &context.alloc._image, &context.alloc._allocation, nullptr));

            VK_CHECK(vkCreateImageView(ci_device, &imageInfo.imageViewInfo, nullptr, &context.view));

            context.sampler = imageInfo.sampler;

            if (imageInfo.sampler == nullptr) {
                auto samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
                vkCreateSampler(ci_device, &samplerInfo, nullptr, &context.sampler);
            }

            ci_resourceIndex[name]        = ci_resourceCounter;
            ci_images[ci_resourceCounter] = context;

            vkinit::debug_object_set_name((uint64_t)ci_images[ci_resourceCounter].alloc._image, VK_OBJECT_TYPE_BUFFER, name.c_str(), this->vkSetDebugUtilsObjectNameEXT, ci_device);

            ci_resourceCounter++;
        }
    };
};
