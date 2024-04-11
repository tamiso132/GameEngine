// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "../common/common_types.h"
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef uint64_t ResourceIndex;

struct VulkanQueue {
    VkQueue  queue;
    uint32_t queueIndex;
};

struct AllocatedBuffer {
    VkBuffer      _buffer;
    VmaAllocation _allocation;
};

struct AllocatedImage {
    VkImage       _image;
    VmaAllocation _allocation;
};

struct TImage {
    VkSampler        sampler;
    VkImageView      view;
    VkFormat         format;
    AllocatedImage   alloc;
    uint8_t          extentIndex;
    VkDescriptorType type;
    VkImageLayout    layout;
    uint16_t         layerCount = 1;
};

struct VulkanContextGlobal {
    VkInstance       instance;
    VkPhysicalDevice chosenGPU;
    VkDevice         device;
    VkExtent2D       windowExtent = {1700, 900};
    // queue used for draw operations
    VulkanQueue graphicQueue;

    // queue used for transfer data and layouts
    VulkanQueue transferQueue;

    // SDL Window
    struct SDL_Window *window{nullptr};

    VkSurfaceKHR   surface;
    VkSwapchainKHR swapchain;

    // our format used for displaying to screen
    VkFormat swapchainImageFormat;

    // images that can be presented to screen
    std::vector<VkImage>     swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    TImage depthImage;

    // global command buffer
    VkCommandBuffer cmd;
    // global pool buffer
    VkCommandPool pool;

    VkDebugUtilsMessengerEXT debug_messenger;
};

struct alignas(16) GPUObject {
    glm::mat4 transformMatrix;
};

struct alignas(16) GPUCamera {
    glm::mat4 viewproj;
    glm::vec3 camPos;
};

struct alignas(16) GPUMaterial {
    glm::vec3    ambient;
    glm::float32 shininess;
    glm::vec3    diffuse;
    char         padding2[4];
    glm::vec3    specular;
};

struct alignas(16) GPUAlignedArrayElement {
    uint32_t faceIndex;
};

struct alignas(16) GPUTexture {
    GPUAlignedArrayElement faceIndices[6]; // 12-byte padding
    GPUMaterial            material;
};

#define VK_CHECK(x)                                                                                                                                                                                    \
    do {                                                                                                                                                                                               \
        VkResult err = x;                                                                                                                                                                              \
        if (err) {                                                                                                                                                                                     \
            std::cout << "Detected Vulkan error: " << err << std::endl;                                                                                                                                \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    } while (0)