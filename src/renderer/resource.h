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
#include <memory>
#include <optional>
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

    struct TextureObjectCreateInfo {
        size_t texWidth;
        size_t texHeight;
        size_t pixelSize;
    };

    struct ImageObjectCreateInfo {
        uint8_t                                bindingNumber;
        VmaAllocationCreateInfo                allocInfo;
        VkImageCreateInfo                      imageInfo;
        VkImageViewCreateInfo                  imageViewInfo;
        std::optional<VkSampler>               sampler;
        std::string                            name;
        void                                  *data;
        size_t                                 dataSize;
        std::optional<TextureObjectCreateInfo> textureAtlas = std::nullopt;
        bool                                   copy         = false;
    };

    struct Blueprint {
        std::vector<BufferObjectCreateInfo> bufferInfos;
        std::vector<ImageObjectCreateInfo>  imageInfos;
    };

    void add_buffer(size_t allocSize, BufferType type, const char *objectName, MemoryType memory = MemoryType::BestFit, const void *data = nullptr, size_t dataSize = 0, bool copy_data = false);

    void add_image_with_view(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, const char *objectName, VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT,
                             VkSampler _Nullable sampler = nullptr, void *data = nullptr, size_t dataSize = 0, size_t pixelSize = 4);

    void add_image_array_with_view(VkFormat format, size_t pixelSize, size_t gridLength, int maxWidth, int maxHeight, std::string name, uint8_t *pixels, bool copy_data = false,
                                   VkSampler sampler = nullptr) {

        VkDeviceSize oneImageSize = pixelSize * gridLength * gridLength;

        VkDeviceSize imageSize = maxWidth * maxHeight * pixelSize;

        uint32_t maxGridX = maxWidth / gridLength;
        uint32_t maxGridY = maxHeight / gridLength;

        TextureObjectCreateInfo atlasInfo;
        atlasInfo.pixelSize = pixelSize;
        atlasInfo.texHeight = maxHeight;
        atlasInfo.texWidth  = maxWidth;

        VmaAllocationCreateInfo allocInfo;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImageCreateInfo vkImageInfo = vkinit::image_create_info(format, 0, VkExtent3D(gridLength, gridLength, 1));
        vkImageInfo.arrayLayers       = (maxHeight / gridLength) * (maxWidth / gridLength);

        VkImageViewCreateInfo viewInfo       = vkinit::imageview_create_info(format, nullptr, VK_IMAGE_ASPECT_COLOR_BIT);
        viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.subresourceRange.layerCount = (maxHeight / gridLength) * (maxWidth / gridLength);

        ImageObjectCreateInfo imageInfo;
        imageInfo.allocInfo     = allocInfo;
        imageInfo.imageInfo     = vkImageInfo;
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
    Blueprint get_blueprint();

  private:
    Blueprint ci_blueprint;
    uint8_t   ci_bindingCounter;
};

struct ResourceContext {

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VkQueue transferQueue, uint32_t transferQueueFamily, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil);

    ImageContext  get_image_by_index(uint64_t blueprintNumber, uint64_t bindingNumber);
    BufferContext get_buffer_by_index(uint64_t blueprintNumber, uint64_t bindingNumber);
    void          get_resource_by_name(std::string resourceName);

    void copy_to_buffer_visible(ResourceIndex dst, size_t size, const void *src);

    void copy_to_buffer_non_visible(ResourceIndex dst, size_t size, const void *src);

    void destroy_buffer(ResourceIndex bufferHandle);

    void copy_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd = nullptr);

    void write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd = nullptr);

    void write_to_texture_array(ResourceBuilder::ImageObjectCreateInfo atlasImageInfo, ImageContext context);

    void build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext, DescriptorAllocatorGrowable descAllocator);

    ResourceIndex build_resource_blueprint(ResourceBuilder::Blueprint resourceContext);

  private:
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
