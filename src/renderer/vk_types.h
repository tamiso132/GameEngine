// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <glm/fwd.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <fstream>
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

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

struct AllocatedBuffer {
    VkBuffer      _buffer;
    VmaAllocation _allocation;
};

struct AllocatedImage {
    VkImage       _image;
    VmaAllocation _allocation;
};

#define VK_CHECK(x)                                                                                                                                                                                    \
    do {                                                                                                                                                                                               \
        VkResult err = x;                                                                                                                                                                              \
        if (err) {                                                                                                                                                                                     \
            std::cout << "Detected Vulkan error: " << err << std::endl;                                                                                                                                \
            abort();                                                                                                                                                                                   \
        }                                                                                                                                                                                              \
    } while (0)