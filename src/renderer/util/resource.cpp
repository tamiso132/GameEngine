#include "vk_resource.h"

#include <cstring>
#include <vulkan/vulkan_core.h>

#include "vk_initializers.h"
#include "vk_types.h"

#include "vkutil.h"

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

static ResourceBuilder::BufferObjectCreateInfo buffer_create_info(size_t allocSize, BufferType type, MemoryType memory = MemoryType::BestFit, const char *objectName = nullptr) {
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

static ResourceBuilder::BufferObjectCreateInfo buffer_staging_info(size_t allocSize) {
    VkMemoryPropertyFlags memoryType;

    memoryType = (VkMemoryPropertyFlags)MemoryType::HostVisible;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage              = (VkBufferUsageFlags)VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.size               = allocSize;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    ResourceBuilder::BufferObjectCreateInfo info;
    info.allocInfo  = allocInfo;
    info.bufferInfo = bufferInfo;

    return info;
}

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
    newbind.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    newbind.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    ci_context.bindings.push_back(newbind);

    ci_context.imageIndices[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::add_buffer(ResourceIndex handle, VkShaderStageFlagBits shaderStage) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = ci_context.bindingCount;
    newbind.descriptorCount = 1;
    newbind.stageFlags      = shaderStage;

    ci_context.bindings.push_back(newbind);

    ci_context.bufferIndices[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::clear() {
    ci_context.bufferIndices.clear();
    ci_context.imageIndices.clear();
    ci_context.bindings.clear();
    ci_context.name         = "";
    ci_context.bindingCount = 0;
}
DescriptorLayoutBuilder::Blueprint DescriptorLayoutBuilder::get_blueprint() { return ci_context; }

/*Resource Builder*/
void ResourceBuilder::add_buffer(size_t allocSize, BufferType type, const char *objectName, MemoryType memory, const void *data, size_t dataSize, bool copy) {
    VkMemoryPropertyFlags memoryType;
    VkBufferCreateInfo    bufferInfo = {};

    if (memory != MemoryType::BestFit) {
        memoryType = (VkMemoryPropertyFlags)memory;
    }

    else {
        memoryType = (VkMemoryPropertyFlags)MemoryType::HostVisible;

        switch (type) {
        case BufferType::Vertex:
        case BufferType::Index:
            memoryType = (VkMemoryPropertyFlags)MemoryType::GpuOnly;
            break;
        }
    }

    if (memoryType == (VkMemoryPropertyFlags)MemoryType::GpuOnly) {
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    VkDescriptorType descriptorType = {};

    switch (type) {
    case BufferType::Storage:
        descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
    case BufferType::Uniform:
        descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    }
    if (descriptorType != 7 && descriptorType != 6) {
        printf("DescriptorType: invalid for %s\n", objectName);
    }

    bufferInfo.usage |= (VkBufferUsageFlags)type;
    bufferInfo.size        = allocSize;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

    if (type == BufferType::Vertex) {
        bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.requiredFlags           = memoryType;

    BufferObjectCreateInfo info = {};
    info.allocInfo              = allocInfo;
    info.bufferInfo             = bufferInfo;

    info.name          = std::string(objectName);
    info.bindingNumber = ci_bindingCounter;
    info.type          = descriptorType;

    if (data != nullptr) {
        size_t size   = allocSize;
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

void ResourceBuilder::add_image_array_with_view(VkFormat format, size_t pixelSize, size_t gridLength, int maxWidth, int maxHeight, std::string name, uint8_t *pixels, bool copy_data,
                                                VkSampler sampler) {
    VkDeviceSize oneImageSize = pixelSize * gridLength * gridLength;

    VkDeviceSize imageSize = maxWidth * maxHeight * pixelSize;

    uint32_t maxGridX = maxWidth / gridLength;
    uint32_t maxGridY = maxHeight / gridLength;

    TextureObjectCreateInfo atlasInfo = {};
    atlasInfo.pixelSize               = pixelSize;
    atlasInfo.texHeight               = maxHeight;
    atlasInfo.texWidth                = maxWidth;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImageCreateInfo vkImageInfo =
        vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VkExtent3D(gridLength, gridLength, 1));
    vkImageInfo.arrayLayers = (maxHeight / gridLength) * (maxWidth / gridLength);
    vkImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkImageViewCreateInfo viewInfo       = vkinit::imageview_create_info(format, nullptr, VK_IMAGE_ASPECT_COLOR_BIT);
    viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.subresourceRange.layerCount = (maxHeight / gridLength) * (maxWidth / gridLength);

    ImageObjectCreateInfo imageInfo;
    imageInfo.allocInfo     = {allocInfo};
    imageInfo.imageInfo     = {vkImageInfo};
    imageInfo.bindingNumber = ci_bindingCounter;
    imageInfo.dataSize      = maxWidth * maxHeight * pixelSize;
    imageInfo.sampler       = sampler ? std::optional(sampler) : std::nullopt;
    imageInfo.name          = name;
    imageInfo.imageViewInfo = viewInfo;
    imageInfo.data          = copy_data ? new uint8_t[imageSize] : pixels;
    imageInfo.textureAtlas  = std::optional(atlasInfo);
    imageInfo.copy          = copy_data;

    ci_blueprint.imageInfos.push_back(imageInfo);
    ci_bindingCounter++;
}

ResourceBuilder::Blueprint ResourceBuilder::get_blueprint() { return ci_blueprint; }

/*Resource Context*/
void ResourceContext::init(VulkanContextGlobal vulkanContext, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil) {

    ci_device        = vulkanContext.device;
    ci_transferQueue = vulkanContext.transferQueue;
    ci_graphicQueue  = vulkanContext.graphicQueue;
    ci_cmdGraphic    = vulkanContext.cmd;

    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(ci_transferQueue.queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(ci_device, &commandPoolInfo, nullptr, &ci_pool));

    VkCommandBufferAllocateInfo bufferInfo = vkinit::command_buffer_allocate_info(ci_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(ci_device, &bufferInfo, &ci_cmdTransfer));

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = vulkanContext.chosenGPU;
    allocatorInfo.device                 = vulkanContext.device;
    allocatorInfo.instance               = vulkanContext.instance;

    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

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
    // TODO create DESCRIPTORALLOCATOR
    VkSamplerCreateInfo blockySampler = vkinit::sampler_create_info(VK_FILTER_NEAREST);

    vkCreateSampler(vulkanContext.device, &blockySampler, nullptr, &ci_default_sampler);
};

void ResourceContext::copy_to_buffer_visible(ResourceIndex dst, size_t size, const void *src) {
    void *dstV;
    vmaMapMemory(ci_allocator, ci_buffers[dst].alloc._allocation, &dstV);
    memcpy(dstV, src, size);
    vmaUnmapMemory(ci_allocator, ci_buffers[dst].alloc._allocation);
}

void ResourceContext::copy_to_buffer_non_visible(ResourceIndex dst, size_t size, const void *src) {
    auto            bufferDst   = ci_buffers[dst];
    auto            stagingInfo = buffer_staging_info(size);
    AllocatedBuffer stagingBuffer;

    VK_CHECK(vmaCreateBuffer(ci_allocator, &stagingInfo.bufferInfo, &stagingInfo.allocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr));

    void *stagingData;
    vmaMapMemory(ci_allocator, stagingBuffer._allocation, &stagingData);
    memcpy(stagingData, src, size);
    vmaUnmapMemory(ci_allocator, stagingBuffer._allocation);

    vkutil::copy_buffer(stagingBuffer._buffer, bufferDst.alloc._buffer, size, ci_cmdTransfer, ci_transferQueue.queue);
}

void ResourceContext::destroy_buffer(ResourceIndex bufferHandle) {
    vmaDestroyBuffer(ci_allocator, ci_buffers[bufferHandle].alloc._buffer, ci_buffers[bufferHandle].alloc._allocation);
    ci_buffers.erase(bufferHandle);
}

void ResourceContext::copy_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd) {

    auto imageRegion = vkinit::image_image_copy_info(ci_extents[ci_images[imageSrc].extentIndex]);

    if (cmd == nullptr) {

        VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkBeginCommandBuffer(ci_cmdTransfer, &beginInfo);

        VkSubmitInfo submit = vkinit::submit_info(&ci_cmdTransfer);
        VK_CHECK(vkQueueSubmit(ci_transferQueue.queue, 1, &submit, nullptr));

        vkQueueWaitIdle(ci_transferQueue.queue);

    } else {
        vkCmdCopyImage(cmd, ci_images[imageSrc].alloc._image, srcLayout, ci_images[imageDst].alloc._image, dstLayout, 1, &imageRegion);
    }
}
// TODO, fix the memory leak, when creating stagingBuffer in non immediate
void ResourceContext::write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd) {

    bool               isImmediate;
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage;
    auto          stagingInfo = buffer_staging_info(sizeOfData);
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
        cmd         = ci_cmdTransfer;

        vkBeginCommandBuffer(cmd, &beginInfo);
    }

    TImage imageDst = ci_images[imageDstHandle];

    vkutil::transition_image_layout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageDst);
    auto copyRegion = vkinit::buffer_image_copy_info(ci_extents[ci_images[imageDstHandle].extentIndex]);

    vkCmdCopyBufferToImage(cmd, ci_buffers[stagingIndex].alloc._buffer, imageDst.alloc._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    vkutil::transition_image_layout(cmd, newLayout, imageDst);

    if (isImmediate) {
        // send it
        vkEndCommandBuffer(cmd);
        auto submitInfo = vkinit::submit_info(&cmd);
        VK_CHECK(vkQueueSubmit(ci_transferQueue.queue, 1, &submitInfo, nullptr));

        vkQueueWaitIdle(ci_transferQueue.queue);
        destroy_buffer(stagingIndex);
        ci_resourceCounter--;
    }
}
void ResourceContext::write_to_texture_array(ResourceBuilder::ImageObjectCreateInfo atlasImageInfo, TImage &context) {

    size_t   gridLength = atlasImageInfo.imageInfo.extent.height;
    size_t   pixelSize  = atlasImageInfo.textureAtlas.value().pixelSize;
    size_t   texWidth   = atlasImageInfo.textureAtlas.value().texWidth;
    size_t   texHeight  = atlasImageInfo.textureAtlas.value().texHeight;
    uint8_t *pixels     = (uint8_t *)atlasImageInfo.data;

    VkDeviceSize oneImageSize = pixelSize * gridLength * gridLength;

    VkDeviceSize imageSize = texWidth * texHeight * pixelSize;

    uint32_t maxGridX = texWidth / gridLength;
    uint32_t maxGridY = texHeight / gridLength;

    auto stageInfo = buffer_staging_info(atlasImageInfo.dataSize);

    AllocatedBuffer stagingBuffer;
    vmaCreateBuffer(ci_allocator, &stageInfo.bufferInfo, &stageInfo.allocInfo, &stagingBuffer._buffer, &stagingBuffer._allocation, nullptr);
    uint32_t bufferOffset = 0;

    char *data;
    vmaMapMemory(ci_allocator, stagingBuffer._allocation, (void **)&data);

    for (int y = 0; y < maxGridY; y++) {
        uint32_t topLeftStart = y * maxGridX * gridLength * gridLength * pixelSize;

        for (int x = 0; x < maxGridX; x++) {
            uint32_t topLeftStartOffset = x * gridLength * pixelSize + topLeftStart;

            for (int i = 0; i < gridLength; i++) {
                uint32_t totalOffset = topLeftStartOffset + i * texWidth * pixelSize;

                memcpy(&data[bufferOffset], &pixels[totalOffset], gridLength * pixelSize);

                bufferOffset += gridLength * pixelSize;
            }
            int x_x = 5;
        }
    }

    vmaUnmapMemory(ci_allocator, stagingBuffer._allocation);
    VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(ci_cmdGraphic, &beginInfo);

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    vkutil::transition_image_layout(ci_cmdGraphic, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, context);
    VkBufferImageCopy copyRegion = vkinit::buffer_image_copy_info(atlasImageInfo.imageInfo.extent, atlasImageInfo.imageInfo.arrayLayers);

    vkCmdCopyBufferToImage(ci_cmdGraphic, stagingBuffer._buffer, context.alloc._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    vkutil::transition_image_layout(ci_cmdGraphic, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, context);

    vkEndCommandBuffer(ci_cmdGraphic);

    auto submitInfo = vkinit::submit_info(&ci_cmdGraphic);
    vkQueueSubmit(ci_graphicQueue.queue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(ci_graphicQueue.queue);

    return vmaDestroyBuffer(ci_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

VkDescriptorSetLayout ResourceContext::build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext, DescriptorAllocatorGrowable descAllocator, VkDescriptorSetLayout layout) {
    VkDescriptorSetLayout new_layout = layout;

    for (int i = 0; i < builderContext.bindings.size(); i++) {
        auto binding = &builderContext.bindings[i];
        if (builderContext.imageIndices.contains(i)) {
            auto imageContext = ci_images[builderContext.imageIndices[i]];

            binding->descriptorType = imageContext.type;
        } else {
            auto bufferContext      = ci_buffers[builderContext.bufferIndices[i]];
            binding->descriptorType = bufferContext.type;
        }
    }

    if (new_layout == nullptr) {
        VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        info.pNext                           = nullptr;
        info.pBindings                       = builderContext.bindings.data();
        info.bindingCount                    = (uint32_t)builderContext.bindings.size();

        VK_CHECK(vkCreateDescriptorSetLayout(ci_device, &info, nullptr, &new_layout));
    }

    auto set = descAllocator.allocate(ci_device, new_layout);
    ci_layouts[new_layout].sets.push_back(set);

    std::vector<VkDescriptorImageInfo> imageInfos = {};
    imageInfos.reserve(builderContext.imageIndices.size() + 1);

    std::vector<VkWriteDescriptorSet> writes = {};
    writes.reserve(builderContext.bindingCount);

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(builderContext.bufferIndices.size() + 1);

    for (int i = 0; i < builderContext.bindings.size(); i++) {
        auto binding = builderContext.bindings[i];

        if (builderContext.imageIndices.contains(i)) {
            auto                  imageContext = ci_images[builderContext.imageIndices[i]];
            VkDescriptorImageInfo info;
            info.sampler     = imageContext.sampler;
            info.imageView   = imageContext.view;
            info.imageLayout = imageContext.layout;

            imageInfos.push_back(info);

            VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

            write.dstBinding      = i;
            write.dstSet          = set; // left empty for now until we need to write it
            write.descriptorCount = 1;
            write.descriptorType  = imageContext.type;
            write.pImageInfo      = &imageInfos[imageInfos.size() - 1];

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
            bufferContext.observerMeta.push_back({new_layout, static_cast<size_t>(i)});
        }
    }

    vkUpdateDescriptorSets(ci_device, writes.size(), writes.data(), 0, nullptr);
    return new_layout;
}

void on_resize(ResourceIndex index){

};
TImage ResourceContext::get_image_by_index(uint64_t blueprintNumber, uint64_t bindingNumber) { return ci_images[blueprintNumber | blueprintNumber >> 56]; }

BufferContext ResourceContext::get_buffer_by_index(uint64_t blueprintNumber, uint64_t bindingNumber) { return ci_buffers[blueprintNumber | blueprintNumber >> 56]; }

ResourceIndex ResourceContext::get_resource_by_name(std::string name) { return ci_resourceIndex[name]; }

ResourceIndex ResourceContext::build_resource_blueprint(ResourceBuilder::Blueprint resourceContext) {
    std::string counterTXT = get_valid_blueprint_counter_name(resourceContext, ci_resourceIndex);

    for (auto &bufferInfo : resourceContext.bufferInfos) {

        BufferContext context{};
        context.usage    = bufferInfo.bufferInfo.usage;
        context.type     = bufferInfo.type;
        context.size     = bufferInfo.bufferInfo.size;
        std::string name = bufferInfo.name + counterTXT;

        VK_CHECK(vmaCreateBuffer(ci_allocator, &bufferInfo.bufferInfo, &bufferInfo.allocInfo, &context.alloc._buffer, &context.alloc._allocation, nullptr));

        vkutil::debug_object_set_name((uint64_t)context.alloc._buffer, VK_OBJECT_TYPE_BUFFER, name.c_str(), ci_device);
        uint64_t      binding       = uint64_t(bufferInfo.bindingNumber) << 56;
        ResourceIndex resourceIndex = binding | ci_resourceCounter;

        ci_resourceIndex[name]    = resourceIndex;
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

        TImage      context = {};
        std::string name    = imageInfo.name + counterTXT;
        context.name        = name;
        context.type        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        context.layout      = VK_IMAGE_LAYOUT_UNDEFINED;
        context.layerCount  = imageInfo.imageInfo.arrayLayers;
        context.format      = imageInfo.imageInfo.format;

        VK_CHECK(vmaCreateImage(ci_allocator, &imageInfo.imageInfo, &imageInfo.allocInfo, &context.alloc._image, &context.alloc._allocation, nullptr));
        imageInfo.imageViewInfo.image = context.alloc._image;

        VK_CHECK(vkCreateImageView(ci_device, &imageInfo.imageViewInfo, nullptr, &context.view));

        if (!imageInfo.sampler.has_value()) {
            context.sampler = ci_default_sampler;
        } else {
            context.sampler = imageInfo.sampler.value();
        }

        ResourceIndex resourceIndex = (uint64_t(imageInfo.bindingNumber) << 56) | ci_resourceCounter;

        ci_resourceIndex[name]   = resourceIndex;
        ci_images[resourceIndex] = context;
        uint64_t objectHandle    = (uint64_t)context.alloc._image;

        vkutil::debug_object_set_name(objectHandle, VK_OBJECT_TYPE_IMAGE, name.c_str(), ci_device);

        if (imageInfo.data != nullptr) {
            if (!imageInfo.textureAtlas.has_value()) {
                write_to_image_from_cpu(resourceIndex, imageInfo.data, imageInfo.dataSize, imageInfo.imageInfo.initialLayout, imageInfo.imageInfo.initialLayout);
            } else {
                // create staging
                write_to_texture_array(imageInfo, ci_images[resourceIndex]);
            }

            if (imageInfo.copy) {
                delete imageInfo.data;
            }
        }
    }

    ci_resourceCounter++;
    return ci_resourceCounter - 1;
}

void test_resource(ResourceContext &context, ResourceIndex imageHandle) {}