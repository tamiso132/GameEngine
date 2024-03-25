
#include "helper.h"
#include <bits/utility.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "../vk_types.h"
#include "vk_initializers.h"

VkDevice Helper::device;
VkPhysicalDeviceProperties Helper::gpuProperties;
VmaAllocator Helper::allocator;
VkCommandBuffer Helper::main_cmd;
VkQueue Helper::graphicQueue;

void Helper::init(VkDevice device, VkPhysicalDeviceProperties gpuProperties, VmaAllocator allocator, VkCommandBuffer cmd, VkQueue graphicQueue) {
    Helper::gpuProperties = gpuProperties;
    Helper::device = device;
    Helper::allocator = allocator;
    Helper::main_cmd = cmd;
    Helper::graphicQueue = graphicQueue;
}

size_t Helper::pad_uniform_buffer_size(size_t originalSize) {
    auto minUboAlignment = Helper::gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t alignedSize = originalSize;
    if (minUboAlignment > 0) {
        alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }
    return alignedSize;
}

AllocatedBuffer Helper::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;

    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;

    AllocatedBuffer newBuffer;

    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));

    return newBuffer;
}

void Helper::transition_image_layout(VkImageLayout oldLayout, VkImageLayout newLayout, VkImage image, VkCommandBuffer cmd) {

    const VkImageMemoryBarrier image_memory_barrier2{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                     .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                     .oldLayout = oldLayout,
                                                     .newLayout = newLayout,
                                                     .image = image,
                                                     .subresourceRange = {
                                                         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                         .baseMipLevel = 0,
                                                         .levelCount = 1,
                                                         .baseArrayLayer = 0,
                                                         .layerCount = 1,
                                                     }};

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
                         0, 0, nullptr, 0, nullptr,
                         1,                     // imageMemoryBarrierCount
                         &image_memory_barrier2 // pImageMemoryBarriers
    );
}

bool Helper::load_shader_module(const char *filePath, VkShaderModule *outShaderModule) {
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
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(Helper::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

std::string Helper::get_filename_from_path(const char *path) {
    const char slash = '/';
    int index = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == slash) {
            index = i;
        }
    }
    index++;

    std::string filename;
    for (int i = 0; i < strlen(path) - index; i++) {
        filename.push_back(path[index + i]);
    }
    printf("filename: %s\n", filename.c_str());
    return filename;
}

void Helper::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
    vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkCommandBuffer cmd = Helper::main_cmd;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vkinit::submit_info(&cmd);

    // submit command buffer to the queue and execute it.
    VK_CHECK(vkQueueSubmit(Helper::graphicQueue, 1, &submit, nullptr));

    vkQueueWaitIdle(Helper::graphicQueue);
}

void Helper::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkResetCommandBuffer(Helper::main_cmd, 0));
    VK_CHECK(vkBeginCommandBuffer(Helper::main_cmd, &cmdBeginInfo));

    // Record the copy command
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(Helper::main_cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(Helper::main_cmd);

    // End the command buffer recording

    // Submit the command buffer for execution (Assuming you have a Vulkan queue)
    VkSubmitInfo submit = vkinit::submit_info(&Helper::main_cmd);

    VK_CHECK(vkQueueSubmit(Helper::graphicQueue, 1, &submit, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(Helper::graphicQueue)); // Optional: Wait for the transfer to complete
}

void Helper::create_image(VkExtent3D extent, VkFormat format, VkImageAspectFlags vkImageAspectFlag, AllocatedImage *image, VkImageView *view) {
    VkImageCreateInfo imageInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, extent);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(Helper::allocator, &imageInfo, &allocInfo, &image->_image, &image->_allocation, nullptr);

    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(format, image->_image, vkImageAspectFlag);

    vkCreateImageView(Helper::device, &dview_info, nullptr, view);
}

void Helper::load_test_image(AllocatedImage &outImage) {
    int texWidth, texHeight, texChannels;

    auto fullpath = std::string(PROJECT_ROOT_PATH) + "/" + "bricks_normal_map.png";

    stbi_uc *pixels = stbi_load(fullpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        // std::cout << "Failed to load texture file " << file << std::endl;
        throw std::runtime_error("failed to load texture!");
    }

    void *pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

    AllocatedBuffer stagingBuffer = Helper::create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(Helper::allocator, stagingBuffer._allocation, &data);

    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));

    vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);
}

void Helper::load_image(const char *file, AllocatedImage &outImage) {
    int texWidth, texHeight, texChannels;

    auto fullpath = std::string(PROJECT_ROOT_PATH) + "/" + file;

    stbi_uc *pixels = stbi_load(fullpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "Failed to load texture file " << file << std::endl;
        throw std::runtime_error("failed to load texture!");
    }

    void *pixel_ptr = pixels;
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

    AllocatedBuffer stagingBuffer = Helper::create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(Helper::allocator, stagingBuffer._allocation, &data);

    memcpy(data, pixel_ptr, static_cast<size_t>(imageSize));

    vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);

    stbi_image_free(pixels);

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texWidth);
    imageExtent.height = static_cast<uint32_t>(texHeight);
    imageExtent.depth = 1;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

    AllocatedImage newImage;

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // allocate and create the image
    vmaCreateImage(Helper::allocator, &dimg_info, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

    Helper::immediate_submit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = newImage._image;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;

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

    std::cout << "Texture loaded succesfully " << file << std::endl;

    outImage = newImage;
}

void Helper::create_cube_map(const char *fileAtlas, uint32_t gridLength, std::vector<std::pair<uint32_t, uint32_t>> cubeMapping, AllocatedImage *cubeMap) {
    int texWidth, texHeight, texChannels;

    auto fullpath = std::string(PROJECT_ROOT_PATH) + "/" + fileAtlas;

    stbi_uc *pixels = stbi_load(fullpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "Failed to load texture atlas " << fileAtlas << std::endl;
        throw std::runtime_error("failed to load texture!");
    }

    uint32_t pixelSize = STBI_rgb_alpha;

    uint32_t cubeMapArraySize = cubeMapping.size() / 6;

    VkDeviceSize imageSize = gridLength * gridLength * pixelSize * 6 * cubeMapArraySize;

    uint32_t maxGridX = texWidth / gridLength;
    uint32_t maxGridY = texHeight / gridLength;

    AllocatedBuffer stagingBuffer = Helper::create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    char *data;
    vmaMapMemory(Helper::allocator, stagingBuffer._allocation, (void **)&data);

    for (int i = 0; i < cubeMapping.size(); i++) {
        uint32_t topLeftStart = (cubeMapping[i].first * gridLength * pixelSize) + cubeMapping[i].second * gridLength * texWidth * pixelSize;

        for (int y = 0; y < gridLength; y++) {
            int rowStart = topLeftStart + y * texWidth * pixelSize;
            int dataOffset = (gridLength * gridLength * pixelSize) * i + y * gridLength * pixelSize;

            memcpy(data + dataOffset, &pixels[rowStart], gridLength * pixelSize);
        }
    }
    vmaUnmapMemory(Helper::allocator, stagingBuffer._allocation);

    stbi_image_free(pixels);

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(gridLength);
    imageExtent.height = static_cast<uint32_t>(gridLength);
    imageExtent.depth = 1;

    VkImageCreateInfo imageInfo = vkinit::image_create_info(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.arrayLayers = cubeMapArraySize * 6;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    imageInfo.pNext = nullptr;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    AllocatedImage newImage;

    vmaCreateImage(Helper::allocator, &imageInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

    Helper::immediate_submit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 6 * cubeMapArraySize;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = newImage._image;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 6 * cubeMapArraySize;
        copyRegion.imageExtent = imageExtent;

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

    *cubeMap = newImage;
}

void Helper::create_texture_array(const char *fileAtlas, uint32_t gridLength, AllocatedImage &imageArray, uint32_t &layers) {
    int texWidth, texHeight, texChannels;

    auto fullpath = std::string(PROJECT_ROOT_PATH) + "/" + fileAtlas;

    stbi_uc *pixels = stbi_load(fullpath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        std::cout << "Failed to load texture atlas " << fileAtlas << std::endl;
        throw std::runtime_error("failed to load texture!");
    }

    uint32_t pixelSize = STBI_rgb_alpha;

    VkDeviceSize oneImageSize = pixelSize * gridLength * gridLength;

    VkDeviceSize imageSize = texWidth * texHeight * pixelSize;

    uint32_t maxGridX = texWidth / gridLength;
    uint32_t maxGridY = texHeight / gridLength;

    AllocatedBuffer stagingBuffer = Helper::create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    char *data;
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

    stbi_image_free(pixels);
    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(gridLength);
    imageExtent.height = static_cast<uint32_t>(gridLength);
    imageExtent.depth = 1;

    layers = (texWidth / gridLength) * (texHeight / gridLength);

    VkImageCreateInfo dimg_info = vkinit::image_create_info(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
    dimg_info.imageType = VK_IMAGE_TYPE_2D;
    dimg_info.arrayLayers = layers;
    dimg_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    dimg_info.pNext = nullptr;
    dimg_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    AllocatedImage newImage;

    // allocate and create the image
    vmaCreateImage(Helper::allocator, &dimg_info, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

    Helper::immediate_submit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = layers;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = newImage._image;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = layers;
        copyRegion.imageExtent = imageExtent;

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