#include "resource.h"
#include "util/helper.h"
#include "vk_mem_alloc.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>

void transition_image_layout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image) {
    const VkImageMemoryBarrier image_memory_barrier2{.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                     .srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                     .oldLayout        = oldLayout,
                                                     .newLayout        = newLayout,
                                                     .image            = image,
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

namespace vkinit {
    VkBufferImageCopy buffer_image_copy_info(VkExtent3D extent) {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset      = 0;
        copyRegion.bufferRowLength   = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = extent;

        return copyRegion;
    }

    VkImageCopy image_image_copy_info(VkExtent3D extent) {
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
        imageRegion.extent    = extent;

        imageRegion.srcSubresource = srcSub;
        imageRegion.dstSubresource = dstSub;

        return imageRegion;
    }
    void debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, PFN_vkSetDebugUtilsObjectNameEXT setDebugName, VkDevice device) {
        VkDebugUtilsObjectNameInfoEXT debugNameInfo = {};
        debugNameInfo.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        debugNameInfo.objectType                    = type;
        debugNameInfo.objectHandle                  = objectHandle;
        debugNameInfo.pObjectName                   = name;

        setDebugName(device, &debugNameInfo);
    };

    ResourceBuilder::BufferObjectCreateInfo buffer_create_info(size_t allocSize, BufferType type, MemoryType memory = MemoryType::BestFit, const char *objectName = nullptr) {
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

        ResourceBuilder::BufferObjectCreateInfo info;
        info.allocInfo  = allocInfo;
        info.bufferInfo = bufferInfo;
        if (objectName != nullptr)
            info.name = std::string(objectName);

        return info;
    };

    ResourceBuilder::BufferObjectCreateInfo buffer_staging_info(size_t allocSize) {
        VkMemoryPropertyFlags memoryType;

        memoryType = (VkMemoryPropertyFlags)MemoryType::HostVisible;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.usage              = (VkBufferUsageFlags)VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.size               = allocSize;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.requiredFlags           = memoryType;

        ResourceBuilder::BufferObjectCreateInfo info;
        info.allocInfo  = allocInfo;
        info.bufferInfo = bufferInfo;

        return info;
    }
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandBuffer cmd, VkQueue queue) {
        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        // Record the copy command
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset    = 0;
        copyRegion.dstOffset    = 0;
        copyRegion.size         = size;

        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        // Submit the command buffer for execution (Assuming you have a Vulkan queue)
        VkSubmitInfo submit = vkinit::submit_info(&cmd);

        VK_CHECK(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(queue)); // Optional: Wait for the transfer to complete
    }
} // namespace vkinit
/*DescriptorAllocator*/
void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios) {
    ratios.clear();

    for (auto r : poolRatios) {
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = create_pool(device, maxSets, poolRatios);

    setsPerPool = maxSets * 1.5; // grow it next allocation

    readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device) {
    for (auto p : readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : fullPools) {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device) {
    for (auto p : readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (auto p : fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}
//< growpool_2

//> growpool_1
VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device) {
    VkDescriptorPool newPool;
    if (readyPools.size() != 0) {
        newPool = readyPools.back();
        readyPools.pop_back();
    } else {
        // need to create a new pool
        newPool = create_pool(device, setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5;
        if (setsPerPool > 4092) {
            setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = uint32_t(ratio.ratio * setCount)});
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = setCount;
    pool_info.poolSizeCount              = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes                 = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
    return newPool;
}
//< growpool_1

//> growpool_3
VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext) {
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = get_pool(device);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext                       = pNext;
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = poolToUse;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &layout;

    VkDescriptorSet ds;
    VkResult        result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    // allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

        fullPools.push_back(poolToUse);

        poolToUse                = get_pool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    readyPools.push_back(poolToUse);
    return ds;
}
//< growpool_3

/*DescriptorLayout Builder*/
void DescriptorLayoutBuilder::add_image(ResourceIndex handle) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = ci_context.bindingCount;
    newbind.descriptorCount = 1;

    ci_context.bindings.push_back(newbind);

    ci_context.imageIndices[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::add_buffer(ResourceIndex handle) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = ci_context.bindingCount;
    newbind.descriptorCount = 1;

    ci_context.bindings.push_back(newbind);

    ci_context.bufferIndices[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::clear() {
    ci_context.bufferIndices.clear();
    ci_context.imageIndices.clear();
    ci_context.bindings.clear();

    ci_context.bindingCount = 0;
}

/*Resource Builder*/
void ResourceBuilder::add_buffer(size_t allocSize, BufferType type, const char *objectName, MemoryType memory, const void *data, size_t dataSize, bool copy) {
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

    BufferObjectCreateInfo info = {};
    info.allocInfo              = allocInfo;
    info.bufferInfo             = bufferInfo;
    info.name                   = std::string(objectName);
    info.bindingNumber          = ci_bindingCounter;

    if (data != nullptr) {
        size_t size   = dataSize ? dataSize : allocSize;
        info.data     = data;
        info.dataSize = size;
        if (copy) {
            void *copiedData = new uint8_t[dataSize];
            memcpy(copiedData, data, size);
            info.data = copiedData;
            info.copy = true;
        }
    }

    ci_blueprint.bufferInfos.push_back(info);
    ci_bindingCounter++;
};

void ResourceBuilder::add_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, const char *objectName, VkImageAspectFlags aspectFlag, VkSampler _Nullable sampler,
                                          void *data, size_t dataSize, size_t pixelSize) {

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
    info.bindingNumber = ci_bindingCounter;
    info.data          = data;
    info.dataSize      = dataSize ? dataSize : (extent.height * extent.width * pixelSize);

    ci_blueprint.imageInfos.push_back(info);
    ci_bindingCounter++;
};

ResourceBuilder::Blueprint ResourceBuilder::get_blueprint() { return ci_blueprint; }

/*Resource Context*/
void ResourceContext::init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VkQueue transferQueue, uint32_t transferQueueFamily, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil) {

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
    DescriptorAllocatorGrowable::PoolSizeRatio poolRatio;

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> poolRatios;

    poolRatio.ratio = 10;
    poolRatio.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    poolRatios.push_back(poolRatio);

    poolRatio.ratio = 10;
    poolRatio.type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolRatios.push_back(poolRatio);

    poolRatio.ratio = 10;
    poolRatio.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

    poolRatios.push_back(poolRatio);
    poolRatio.ratio = 10;
    poolRatio.type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;

    poolRatios.push_back(poolRatio);
    poolRatio.ratio = 10;
    poolRatio.type  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolRatios.push_back(poolRatio);

    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &ci_allocator));
};

void ResourceContext::copy_to_buffer_visible(ResourceIndex dst, size_t size, const void *src) {
    void *dstV;
    vmaMapMemory(ci_allocator, ci_buffers[dst].alloc._allocation, &dstV);
    memcpy(dstV, src, size);
    vmaUnmapMemory(ci_allocator, ci_buffers[dst].alloc._allocation);
}

void ResourceContext::copy_to_buffer_non_visible(ResourceIndex dst, size_t size, const void *src) {
    auto            bufferDst   = ci_buffers[dst];
    auto            stagingInfo = vkinit::buffer_staging_info(size);
    AllocatedBuffer stagingBuffer;

    VK_CHECK(vmaCreateBuffer(ci_allocator, &stagingInfo.bufferInfo, &stagingInfo.allocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

    void *stagingData;
    vmaMapMemory(ci_allocator, stagingBuffer._allocation, &stagingData);
    memcpy(stagingData, src, size);
    vmaUnmapMemory(ci_allocator, stagingBuffer._allocation);

    vkinit::copy_buffer(stagingBuffer._buffer, bufferDst.alloc._buffer, size, ci_cmd, ci_transferQueue);
}

void ResourceContext::destroy_buffer(ResourceIndex bufferHandle) {
    vmaDestroyBuffer(ci_allocator, ci_buffers[bufferHandle].alloc._buffer, ci_buffers[bufferHandle].alloc._allocation);
    ci_buffers.erase(bufferHandle);
}

void ResourceContext::copy_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd) {

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
// TODO, fix the memory leak, when creating stagingBuffer in non immediate
void ResourceContext::write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd) {

    bool               isImmediate;
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage;
    auto          stagingInfo = vkinit::buffer_staging_info(sizeOfData);
    BufferContext bufferContext;
    VK_CHECK(vmaCreateBuffer(ci_allocator, &stagingInfo.bufferInfo, &stagingInfo.allocInfo, &bufferContext.alloc._buffer, &bufferContext.alloc._allocation, nullptr));

    bufferContext.size  = stagingInfo.bufferInfo.size;
    bufferContext.type  = (VkDescriptorType)0;
    bufferContext.usage = stagingInfo.bufferInfo.usage;

    ci_buffers[ci_resourceCounter] = bufferContext;
    ResourceIndex stagingIndex     = ci_resourceCounter;
    ci_resourceCounter++;

    copy_to_buffer_visible(stagingIndex, sizeOfData, src);
    auto beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    if (cmd == nullptr) {
        isImmediate = true;
        cmd         = ci_cmd;

        vkBeginCommandBuffer(cmd, &beginInfo);
    }

    auto imageDst = ci_images[imageDstHandle];

    transition_image_layout(cmd, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageDstHandle);
    auto copyRegion = vkinit::buffer_image_copy_info(ci_extents[ci_images[imageDstHandle].extentIndex]);

    vkCmdCopyBufferToImage(cmd, ci_buffers[stagingIndex].alloc._buffer, imageDst.alloc._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    transition_image_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout, imageDstHandle);

    if (isImmediate) {
        // send it
        vkEndCommandBuffer(cmd);
        auto submitInfo = vkinit::submit_info(&cmd);
        VK_CHECK(vkQueueSubmit(ci_transferQueue, 1, &submitInfo, nullptr));

        vkQueueWaitIdle(ci_transferQueue);
        destroy_buffer(stagingIndex);
    }
}

void ResourceContext::build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext, DescriptorAllocatorGrowable descAllocator) {

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext                           = nullptr;

    info.pBindings    = builderContext.bindings.data();
    info.bindingCount = (uint32_t)builderContext.bindings.size();
    info.flags        = builderContext.shaderStage;

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(ci_device, &info, nullptr, &setLayout));

    auto set = descAllocator.allocate(ci_device, setLayout);

    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet>  writes;

    std::vector<VkDescriptorBufferInfo> bufferInfos;

    for (int i = 0; i < builderContext.bindings.size(); i++) {
        auto binding = builderContext.bindings[i];

        if (builderContext.imageIndices.contains(i)) {
            auto                  imageContext = ci_images[builderContext.imageIndices[i]];
            VkDescriptorImageInfo info         = VkDescriptorImageInfo{.sampler = imageContext.sampler, .imageView = imageContext.view, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED};

            imageInfos.push_back(info);

            VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

            write.dstBinding      = i;
            write.dstSet          = set; // left empty for now until we need to write it
            write.descriptorCount = 1;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo      = &info;

            writes.push_back(write);
        } else {
            auto                    bufferContext = ci_buffers[builderContext.bufferIndices[i]];
            VkDescriptorBufferInfo &info          = bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = bufferContext.alloc._buffer, .offset = 0, .range = bufferContext.size});

            VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

            write.dstBinding      = i;
            write.dstSet          = set; // left empty for now until we need to write it
            write.descriptorCount = 1;
            write.descriptorType  = bufferContext.type;
            write.pBufferInfo     = &info;

            writes.push_back(write);
        }
    }

    vkUpdateDescriptorSets(ci_device, writes.size(), writes.data(), 0, nullptr);
}

ResourceIndex ResourceContext::build_resource_blueprint(ResourceBuilder::Blueprint resourceContext) {
    std::string counterTXT = get_valid_blueprint_counter_name(resourceContext, ci_resourceIndex);

    uint64_t blueprintNumber = ci_resourceCounter & 0x0FFFFFFFFFFFFFFF;
    uint64_t bindingNumber;

    for (auto &bufferInfo : resourceContext.bufferInfos) {

        BufferContext context;
        context.usage = bufferInfo.bufferInfo.usage;

        std::string name = bufferInfo.name + counterTXT;
        vmaCreateBuffer(ci_allocator, &bufferInfo.bufferInfo, &bufferInfo.allocInfo, &context.alloc._buffer, &context.alloc._allocation, nullptr);

        vkinit::debug_object_set_name((uint64_t)ci_buffers[ci_resourceCounter].alloc._buffer, VK_OBJECT_TYPE_BUFFER, name.c_str(), this->vkSetDebugUtilsObjectNameEXT, ci_device);

        ResourceIndex resourceIndex = (bufferInfo.bindingNumber << 56) | ci_resourceCounter;

        ci_resourceIndex[name]    = ci_resourceCounter;
        ci_buffers[resourceIndex] = context;

        if (bufferInfo.data != nullptr) {
            if ((bufferInfo.allocInfo.requiredFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                copy_to_buffer_visible(resourceIndex, bufferInfo.dataSize, bufferInfo.data);
            } else {
                copy_to_buffer_non_visible(resourceIndex, bufferInfo.dataSize, bufferInfo.data);
            }

            if (bufferInfo.copy) {
                delete bufferInfo.data;
            }
        }
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

        ResourceIndex resourceIndex = (imageInfo.bindingNumber << 56) | ci_resourceCounter;

        ci_resourceIndex[name]   = ci_resourceCounter;
        ci_images[resourceIndex] = context;

        vkinit::debug_object_set_name((uint64_t)ci_images[ci_resourceCounter].alloc._image, VK_OBJECT_TYPE_BUFFER, name.c_str(), this->vkSetDebugUtilsObjectNameEXT, ci_device);

        if (imageInfo.data != nullptr) {
            write_to_image_from_cpu(resourceIndex, imageInfo.data, imageInfo.dataSize, imageInfo.imageInfo.initialLayout, imageInfo.imageInfo.initialLayout);
        }
    }

    ci_resourceCounter++;
    return ci_resourceCounter - 1;
}
