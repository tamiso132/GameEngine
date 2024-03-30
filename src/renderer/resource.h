#pragma once

#include "VkBootstrapDispatch.h"
#include "util/helper.h"
#include "util/vk_initializers.h"
#include "vk_mem_alloc.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
// 56 bits for blueprint number
// 8 bits for bindings
typedef uint64_t ResourceIndex;

void transition_image_layout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout, ResourceIndex image);

namespace vkinit {
    VkBufferImageCopy buffer_image_copy_info(VkExtent3D extent);
    VkImageCopy       image_image_copy_info(VkExtent3D extent);
    void              debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, PFN_vkSetDebugUtilsObjectNameEXT setDebugName, VkDevice device);
} // namespace vkinit

enum class BufferType {
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
    VkSampler        sampler;
    VkImageView      view;
    AllocatedImage   alloc;
    uint8_t          extentIndex;
    VkDescriptorType type;
};

struct BufferContext {
    AllocatedBuffer    alloc;
    VkBufferUsageFlags usage;
    VkDescriptorType   type;
    size_t             size;
};

struct DescriptorAllocatorGrowable {
  public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float            ratio;
    };

    void init(VkDevice device, uint32_t initialSets, std::span<PoolSizeRatio> poolRatios);
    void clear_pools(VkDevice device);
    void destroy_pools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);

  private:
    VkDescriptorPool get_pool(VkDevice device);
    VkDescriptorPool create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio>    ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t                      setsPerPool;
};

//> descriptor_layout
struct DescriptorLayoutBuilder {

    void add_binding(uint32_t binding, VkDescriptorType type);
    void add_buffer(ResourceIndex handle);
    void add_image(ResourceIndex handle);
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
        uint8_t                 bindingNumber;
        VkBufferCreateInfo      bufferInfo;
        VmaAllocationCreateInfo allocInfo;
        std::string             name;
        const void             *data;
        size_t                  dataSize;
        bool                    copy;
    };

    struct ImageObjectCreateInfo {
        uint8_t                 bindingNumber;
        VmaAllocationCreateInfo allocInfo;
        VkImageCreateInfo       imageInfo;
        VkImageViewCreateInfo   imageViewInfo;
        VkSampler               sampler;
        std::string             name;
        void                   *data;
        size_t                  dataSize;
    };
    struct Blueprint {
        std::vector<BufferObjectCreateInfo> bufferInfos;
        std::vector<ImageObjectCreateInfo>  imageInfos;
    };

    void add_buffer(size_t allocSize, BufferType type, const char *objectName, MemoryType memory = MemoryType::BestFit, const void *data = nullptr, size_t dataSize = 0, bool copy_data = false);

    void add_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, const char *objectName, VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT,
                             VkSampler _Nullable sampler = nullptr, void *data = nullptr, size_t dataSize = 0, size_t pixelSize = 4);

    void add_image_array_with_view(VkFormat format, size_t pixelSize, size_t gridLength, int maxWidth, int maxHeight, void *data) {

        VkDeviceSize oneImageSize = pixelSize * gridLength * gridLength;

        VkDeviceSize imageSize = maxWidth * maxHeight * pixelSize;

        uint32_t              maxGridX      = maxWidth / gridLength;
        uint32_t              maxGridY      = maxHeight / gridLength;
        AllocatedBuffer       stagingBuffer = Helper::create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        ImageObjectCreateInfo imageContext;

        vmaMapMemory(Helper::allocator, stagingBuffer._allocation, (void **)&data);
        uint32_t bufferOffset = 0;

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

        vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);

        VkExtent3D imageExtent;
        imageExtent.width  = static_cast<uint32_t>(gridLength);
        imageExtent.height = static_cast<uint32_t>(gridLength);
        imageExtent.depth  = 1;

        auto layers = (maxWidth / gridLength) * (maxHeight / gridLength);

        imageContext.imageInfo             = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
        imageContext.imageInfo.imageType   = VK_IMAGE_TYPE_2D;
        imageContext.imageInfo.arrayLayers = layers;

        imageContext.allocInfo       = {};
        imageContext.allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        imageContext.bindingNumber = ci_bindingCounter;
        AllocatedImage newImage;

        // allocate and create the image

        Helper::immediate_submit([&](VkCommandBuffer cmd) {
            VkImageSubresourceRange range;
            range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel   = 0;
            range.levelCount     = 1;
            range.baseArrayLayer = 0;
            range.layerCount     = layers;

            VkImageMemoryBarrier imageBarrier_toTransfer = {};
            imageBarrier_toTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

            imageBarrier_toTransfer.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier_toTransfer.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toTransfer.image            = newImage._image;
            imageBarrier_toTransfer.subresourceRange = range;

            imageBarrier_toTransfer.srcAccessMask = 0;
            imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            // barrier the image into the transfer-receive layout
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset      = 0;
            copyRegion.bufferRowLength   = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel       = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount     = layers;
            copyRegion.imageExtent                     = imageExtent;

            // copy the buffer into the image

            vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

            imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // barrier the image into the shader readable layout
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
        });

        vmaDestroyBuffer(Helper::allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        imageArray = newImage;
    }
    Blueprint get_blueprint();

  private:
    Blueprint ci_blueprint;
    uint8_t   ci_bindingCounter;
};

struct ResourceContext {

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VkQueue transferQueue, uint32_t transferQueueFamily, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil);

    void get_resource_by_index(uint64_t blueprintNumber, uint8_t bindingNumber);

    void get_resource_by_name(std::string resourceName);

    void copy_to_buffer_visible(ResourceIndex dst, size_t size, const void *src);

    void copy_to_buffer_non_visible(ResourceIndex dst, size_t size, const void *src);

    void destroy_buffer(ResourceIndex bufferHandle);

    void copy_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd = nullptr);

    void write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd = nullptr);

    void build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext, DescriptorAllocatorGrowable descAllocator);

    ResourceIndex build_resource_blueprint(ResourceBuilder::Blueprint resourceContext);

  private:
    void create_buffer(size_t allocSize, VkBufferUsageFlags usage);

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
};
