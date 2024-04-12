#include "vk_initializers.h"
#include <vulkan/vulkan_core.h>

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/) {
    VkCommandPoolCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext                   = nullptr;

    info.flags            = flags;
    info.queueFamilyIndex = queueFamilyIndex;
    return info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count /*= 1*/, VkCommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/) {
    VkCommandBufferAllocateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext                       = nullptr;

    info.commandPool        = pool;
    info.commandBufferCount = count;
    info.level              = level;
    return info;
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/) {
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext                    = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags            = flags;
    return info;
}

VkFramebufferCreateInfo vkinit::framebuffer_create_info(VkRenderPass renderPass, VkExtent2D extent) {
    VkFramebufferCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext                   = nullptr;

    info.renderPass      = renderPass;
    info.attachmentCount = 1;
    info.width           = extent.width;
    info.height          = extent.height;
    info.layers          = 1;

    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags /*= 0*/) {
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/) {
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext                 = nullptr;
    info.flags                 = flags;
    return info;
}

VkSubmitInfo vkinit::submit_info(VkCommandBuffer *cmd) {
    VkSubmitInfo info = {};
    info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext        = nullptr;

    info.waitSemaphoreCount   = 0;
    info.pWaitSemaphores      = nullptr;
    info.pWaitDstStageMask    = nullptr;
    info.commandBufferCount   = 1;
    info.pCommandBuffers      = cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores    = nullptr;

    return info;
}

VkPresentInfoKHR vkinit::present_info() {
    VkPresentInfoKHR info = {};
    info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext            = nullptr;

    info.swapchainCount     = 0;
    info.pSwapchains        = nullptr;
    info.pWaitSemaphores    = nullptr;
    info.waitSemaphoreCount = 0;
    info.pImageIndices      = nullptr;

    return info;
}

VkRenderPassBeginInfo vkinit::renderpass_begin_info(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer) {
    VkRenderPassBeginInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext                 = nullptr;

    info.renderPass          = renderPass;
    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent   = windowExtent;
    info.clearValueCount     = 1;
    info.pClearValues        = nullptr;
    info.framebuffer         = framebuffer;

    return info;
}

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    // shader stage
    info.stage = stage;
    // module containing the code for this shader stage
    info.module = shaderModule;
    // the entry point of the shader
    info.pName = "main";
    return info;
}
VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext                                = nullptr;

    // no vertex bindings or attributes
    info.vertexBindingDescriptionCount   = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext                                  = nullptr;

    info.topology = topology;
    // we are not going to use primitive restart on the entire tutorial so leave
    // it on false
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}
VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode) {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext                                  = nullptr;

    info.depthClampEnable = VK_FALSE;
    // rasterizer discard allows objects with holes, default to no
    info.rasterizerDiscardEnable = VK_FALSE;

    info.polygonMode = polygonMode;
    info.lineWidth   = 1.0f;
    // no backface cull
    info.cullMode  = VK_CULL_MODE_NONE;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // no depth bias
    info.depthBiasEnable         = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp          = 0.0f;
    info.depthBiasSlopeFactor    = 0.0f;

    return info;
}
VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info() {
    VkPipelineMultisampleStateCreateInfo info = {};
    info.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext                                = nullptr;

    info.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    info.rasterizationSamples  = VK_SAMPLE_COUNT_4_BIT;
    info.minSampleShading      = 1.0f;
    info.pSampleMask           = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable      = VK_FALSE;
    return info;
}
VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask                      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable                         = VK_FALSE;
    return colorBlendAttachment;
}
VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty defaults
    info.flags                  = 0;
    info.setLayoutCount         = 0;
    info.pSetLayouts            = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges    = nullptr;
    return info;
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext             = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = extent;

    info.mipLevels   = 1;
    info.arrayLayers = 1;
    info.samples     = VK_SAMPLE_COUNT_1_BIT;
    info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    info.usage       = usageFlags;

    return info;
}

VkImageViewCreateInfo vkinit::imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    // build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext                 = nullptr;

    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.image                           = image;
    info.format                          = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    info.subresourceRange.aspectMask     = aspectFlags;

    return info;
}

VkPipelineDepthStencilStateCreateInfo vkinit::depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp) {
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext                                 = nullptr;

    info.depthTestEnable       = bDepthTest ? VK_TRUE : VK_FALSE;
    info.depthWriteEnable      = bDepthWrite ? VK_TRUE : VK_FALSE;
    info.depthCompareOp        = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds        = 0.0f; // Optional
    info.maxDepthBounds        = 1.0f; // Optional
    info.stencilTestEnable     = VK_FALSE;

    return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding                      = binding;
    setbind.descriptorCount              = 1;
    setbind.descriptorType               = type;
    setbind.pImmutableSamplers           = nullptr;
    setbind.stageFlags                   = stageFlags;

    return setbind;
}
VkWriteDescriptorSet vkinit::write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext                = nullptr;

    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = bufferInfo;

    return write;
}

VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext                = nullptr;

    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = imageInfo;

    return write;
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAdressMode /*= VK_SAMPLER_ADDRESS_MODE_REPEAT*/) {
    VkSamplerCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext               = nullptr;

    info.magFilter    = filters;
    info.minFilter    = filters;
    info.addressModeU = samplerAdressMode;
    info.addressModeV = samplerAdressMode;
    info.addressModeW = samplerAdressMode;

    return info;
}

namespace vkinit {

    VkRenderingAttachmentInfo render_attachment_info(VkClearValue clearValue, VkImageLayout layout, VkImageView view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp) {
        VkRenderingAttachmentInfo attachment = {};

        attachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment.clearValue  = clearValue;
        attachment.imageLayout = layout;
        attachment.imageView   = view;
        attachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.resolveMode = VK_RESOLVE_MODE_NONE;

        return attachment;
    }

    VkRenderingInfoKHR render_info_khr(VkRenderingAttachmentInfo &colorAttachment, VkRenderingAttachmentInfo *depthAttachment, VkExtent2D windowExtent) {
        VkRenderingInfoKHR dynamicInfo   = {};
        dynamicInfo.pNext                = nullptr;
        dynamicInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
        dynamicInfo.colorAttachmentCount = 1;
        dynamicInfo.pColorAttachments    = &colorAttachment;
        dynamicInfo.pDepthAttachment     = depthAttachment;
        dynamicInfo.renderArea           = {{0, 0}, windowExtent};
        dynamicInfo.layerCount           = 1;

        return dynamicInfo;
    }

    VkImageMemoryBarrier image_barrier_info(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess) {

        VkImageSubresourceRange subresourceRange = {};

        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VkImageMemoryBarrier barrier = {};

        barrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout        = oldLayout;
        barrier.newLayout        = newLayout;
        barrier.image            = image;
        barrier.srcAccessMask    = srcAccess;
        barrier.dstAccessMask    = dstAccess;
        barrier.subresourceRange = subresourceRange;

        return barrier;
    };

    VkBufferImageCopy buffer_image_copy_info(VkExtent3D extent, uint32_t layerCount) {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset      = 0;
        copyRegion.bufferRowLength   = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = layerCount;
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

    VkPipelineColorBlendStateCreateInfo color_blend_info(const std::vector<VkPipelineColorBlendAttachmentState> &colorBlendStates) {
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.pNext                               = nullptr;

        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.logicOp         = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = colorBlendStates.size();
        colorBlending.pAttachments    = colorBlendStates.data();
        return colorBlending;
    }

    VkPipelineViewportStateCreateInfo viewport_state_info(VkViewport *viewport, VkRect2D *scissor) {

        VkPipelineViewportStateCreateInfo viewportState = {};

        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = scissor;

        return viewportState;
    }

    VkPipelineMultisampleStateCreateInfo multi_sample_info(VkSampleCountFlagBits sample_count, VkBool32 shadingSample) {

        VkPipelineMultisampleStateCreateInfo multisampleInfo = {};

        multisampleInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = sample_count;
        multisampleInfo.sampleShadingEnable  = shadingSample; // You can set this to VK_TRUE if you want to enable sample shading

        return multisampleInfo;
    }

    VkPipelineRenderingCreateInfoKHR pipeline_render_create_info_khr(VkFormat &colorFormat, VkFormat depthFormat) {
        VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
        pipelineCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineCreateInfo.pNext                            = nullptr;
        pipelineCreateInfo.pColorAttachmentFormats          = &colorFormat;
        pipelineCreateInfo.colorAttachmentCount             = 1;
        pipelineCreateInfo.depthAttachmentFormat            = depthFormat;

        return pipelineCreateInfo;
    }
} // namespace vkinit