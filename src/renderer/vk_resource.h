#pragma once
#include "common/std_common.h"
#include <optional>
#include <span>

#include "vk_types.h"

// 56 bits for blueprint number
// 8 bits for bindings

namespace vkinit {
    VkBufferImageCopy    buffer_image_copy_info(VkExtent3D extent, uint32_t layerCount = 1);
    VkImageCopy          image_image_copy_info(VkExtent3D extent);
    VkImageMemoryBarrier image_barrier_info(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess);
} // namespace vkinit

namespace util {
    void debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, VkDevice device);
    void transition_image_color_attachment(VkCommandBuffer cmd,VkImage image);
    void transition_image_present(VkCommandBuffer cmd, VkImage image);
} // namespace util

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
    VkImageLayout    layout;
    uint16_t         layerCount = 1;
};

struct ObserverMeta {
    VkDescriptorSetLayout layout;
    size_t                binding;
};

struct BufferContext {
    AllocatedBuffer    alloc;
    VkBufferUsageFlags usage;
    VkDescriptorType   type;
    size_t             size;

    std::vector<ObserverMeta> observerMeta;
};

struct LayoutContext {
    std::vector<VkDescriptorSet> sets;
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

    void add_buffer(ResourceIndex handle, VkShaderStageFlagBits shaderStage);
    void add_image(ResourceIndex handle);
    void clear();

    struct Blueprint {

        uint32_t                                    bindingCount = 0;
        std::vector<VkDescriptorSetLayoutBinding>   bindings;
        std::unordered_map<uint32_t, ResourceIndex> bufferIndices;
        std::unordered_map<uint32_t, ResourceIndex> imageIndices;
        std::string                                 name;
    };

    // VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
    Blueprint get_blueprint();

  private:
    void add_binding(uint32_t binding, VkDescriptorType type);

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
        VkDescriptorType        type;
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

    void      add_image_array_with_view(VkFormat format, size_t pixelSize, size_t gridLength, int maxWidth, int maxHeight, std::string name, uint8_t *pixels, bool copy_data = false,
                                        VkSampler sampler = nullptr);
    Blueprint get_blueprint();

  private:
    Blueprint ci_blueprint;
    uint8_t   ci_bindingCounter = 0;
};

struct ResourceContext {

    void init(VkPhysicalDevice physical, VkDevice device, VkInstance instance, VulkanQueue transferQueue, VulkanQueue graphicQueue, VkCommandBuffer cmdGraphic,
              PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtil);

    void copy_to_buffer_visible(ResourceIndex dst, size_t size, const void *src);

    void copy_to_buffer_non_visible(ResourceIndex dst, size_t size, const void *src);

    void destroy_buffer(ResourceIndex bufferHandle);

    void copy_to_image(ResourceIndex imageSrc, ResourceIndex imageDst, VkImageLayout srcLayout, VkImageLayout dstLayout, VkCommandBuffer cmd = nullptr);

    void write_to_image_from_cpu(ResourceIndex imageDstHandle, void *src, size_t sizeOfData, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmd = nullptr);

    void write_to_texture_array(ResourceBuilder::ImageObjectCreateInfo atlasImageInfo, ImageContext &context);

    VkDescriptorSetLayout build_descriptor_layout_blueprint(DescriptorLayoutBuilder::Blueprint builderContext, DescriptorAllocatorGrowable descAllocator, VkDescriptorSetLayout layout = nullptr);

    ResourceIndex build_resource_blueprint(ResourceBuilder::Blueprint resourceContext);

    ImageContext  get_image_by_index(uint64_t blueprintNumber, uint64_t bindingNumber);
    BufferContext get_buffer_by_index(uint64_t blueprintNumber, uint64_t bindingNumber);
    ResourceIndex get_resource_by_name(std::string resourceName);

  private:
    VmaAllocator            ci_allocator;
    std::vector<VkExtent3D> ci_extents;

    VulkanQueue ci_transferQueue;
    VulkanQueue ci_graphicQueue;

    std::unordered_map<std::string, ResourceIndex>           ci_resourceIndex;
    std::unordered_map<ResourceIndex, BufferContext>         ci_buffers;
    std::unordered_map<ResourceIndex, ImageContext>          ci_images;
    std::unordered_map<VkDescriptorSetLayout, LayoutContext> ci_layouts;

    VkSampler ci_default_sampler;
    VkDevice  ci_device;

    VkCommandBuffer ci_cmdTransfer;
    VkCommandBuffer ci_cmdGraphic;
    VkCommandPool   ci_pool;
    uint32_t        ci_resourceCounter = 0;
};
