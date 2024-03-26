#include "resource.h"
#include <vulkan/vulkan_core.h>
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
} // namespace vkinit
// define all descriptorlayoutbuilder
/* void add_binding(uint32_t binding, VkDescriptorType type);
    void add_buffer(BufferHandle handle);
    void add_image(ImageHandle handle);
    void clear();*/
void DescriptorLayoutBuilder::add_image(ImageHandle handle) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = ci_context.bindingCount;
    newbind.descriptorCount = 1;

    ci_context.bindings.push_back(newbind);

    ci_context.imageBindings[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::add_buffer(BufferHandle handle) {
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = ci_context.bindingCount;
    newbind.descriptorCount = 1;

    ci_context.bindings.push_back(newbind);

    ci_context.buffersBindings[ci_context.bindingCount] = handle;
    ci_context.bindingCount++;
}

void DescriptorLayoutBuilder::clear() {
    ci_context.buffersBindings.clear();
    ci_context.imageBindings.clear();
    ci_context.bindings.clear();

    ci_context.bindingCount = 0;
}
