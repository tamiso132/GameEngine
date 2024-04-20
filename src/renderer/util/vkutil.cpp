#include "vkutil.h"

#include <fstream>
#include <vulkan/vulkan_core.h>

#include "vk_initializers.h"

#define NDebug
namespace vkutil {

    VkCommandBuffer create_cmd(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPool pool) {
        VkCommandBuffer cmd;

        VkCommandBufferAllocateInfo bufferInfo = vkinit::command_buffer_allocate_info(pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(context.device, &bufferInfo, &cmd));

        return cmd;
    };

    VkCommandPool create_pool(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag) {
        VkCommandPool pool;

        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(queueFamilyIndex, flag);
        VK_CHECK(vkCreateCommandPool(context.device, &commandPoolInfo, nullptr, &pool));

        return pool;
    }

    VkFence create_fence(VulkanContextGlobal context) {
        VkFence fence;

        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        vkCreateFence(context.device, &fenceCreateInfo, nullptr, &fence);

        return fence;
    }

    VkSemaphore create_semaphore(VulkanContextGlobal context) {
        VkSemaphore semaphore;

        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
        vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &semaphore);

        return semaphore;
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

    void transition_image_present(VkCommandBuffer cmd, VkImage image) {

        VkAccessFlags srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkAccessFlags dstAccess = VK_ACCESS_MEMORY_READ_BIT;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkImageMemoryBarrier barrier = vkinit::image_barrier_info(image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, srcAccess, dstAccess);
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    };

    void transition_image_color_attachment(VkCommandBuffer cmd, VkImage image) {

        VkAccessFlags srcAccess = VK_ACCESS_NONE_KHR;
        VkAccessFlags dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkImageMemoryBarrier barrier = vkinit::image_barrier_info(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, srcAccess, dstAccess);
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void transition_image_layout(VkCommandBuffer cmd, VkImageLayout newLayout, TImage &imageContext) {

        VkImageMemoryBarrier barrier = VkImageMemoryBarrier{.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                            .srcAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                            .oldLayout        = imageContext.layout,
                                                            .newLayout        = newLayout,
                                                            .image            = imageContext.alloc._image,
                                                            .subresourceRange = {
                                                                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                .baseMipLevel   = 0,
                                                                .levelCount     = 1,
                                                                .baseArrayLayer = 0,
                                                                .layerCount     = imageContext.layerCount,
                                                            }};

        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;

        if (VK_IMAGE_LAYOUT_UNDEFINED == imageContext.layout && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {

            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            // earliest stage of the pipeline
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            // Transfer stage
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == imageContext.layout && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // From a copying stage to...
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            // The fragment stage.
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        } else if (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR == newLayout) {

            // From color attachment to Present
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

            // Write to swapchain stage
            srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            // done with everything
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        else if (imageContext.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {

            // From Present back to Color attachment
            barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        vkCmdPipelineBarrier(cmd,
                             srcStage, // srcStageMask
                             dstStage, // dstStageMask
                             0, 0, nullptr, 0, nullptr,
                             1,       // imageMemoryBarrierCount
                             &barrier // pImageMemoryBarriers
        );
        imageContext.layout = newLayout;
    }

    bool load_shader_module(VkDevice device, const char *filePath, VkShaderModule *outShaderModule) {
        std::string full_path = std::string(PROJECT_ROOT_PATH) + "/" + filePath;

        // open the file. With cursor at the end
        std::ifstream file(full_path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return false;
        }

        // find what the size of the file is by looking up the location of the cursor
        // because the cursor is at the end, it gives the size directly in bytes
        size_t fileSize = (size_t)file.tellg();

        // spirv expects the buffer to be on uint32, so make sure to reserve a int
        // vector big enough for the entire file
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        // put file cursor at beggining
        file.seekg(0);

        // load the entire file into the buffer
        file.read((char *)buffer.data(), fileSize);

        // now that the file is loaded into the buffer, we can close it
        file.close();

        // create a new shader module, using the buffer we loaded
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext                    = nullptr;

        // codeSize has to be in bytes, so multply the ints in the buffer by size of
        // int to know the real size of the buffer
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode    = buffer.data();

        // check that the creation goes well.
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }

    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT feature_dynamic_unusued_attachment_EXT() {
        VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT info{};
        info.dynamicRenderingUnusedAttachments = VK_TRUE;
        info.sType                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT;

        return info;
    }

    VkPhysicalDeviceBufferDeviceAddressFeatures feature_buffer_device_adress() {
        VkPhysicalDeviceBufferDeviceAddressFeatures info{};
        info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

        return info;
    }

#if defined(NDebug)
    void debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, VkDevice device) {
        VkDebugUtilsObjectNameInfoEXT debugNameInfo = {};
        debugNameInfo.sType                         = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        debugNameInfo.objectType                    = type;
        debugNameInfo.objectHandle                  = objectHandle;
        debugNameInfo.pObjectName                   = name;

        cmdSetDebugUtilsObjectNameEXT(device, &debugNameInfo);
    };
#else
    void debug_object_set_name(uint64_t objectHandle, VkObjectType type, const char *name, VkDevice device){};
#endif
} // namespace vkutil