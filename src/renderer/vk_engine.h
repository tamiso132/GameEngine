﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "descriptor.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "../camera/camera.h"
#include "util/vk_descriptors.h"

#include "vk_create.h"
#include "vk_mesh.h"
#include "vk_types.h"

#include <VkBootstrap.h>

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

struct Texture {
    AllocatedImage image;
    VkImageView    imageView;
};

class PipelineBuilder {
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo         _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo       _inputAssembly;
    VkViewport                                   _viewport;
    VkRect2D                                     _scissor;
    VkPipelineRasterizationStateCreateInfo       _rasterizer;
    VkPipelineColorBlendAttachmentState          _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo         _multisampling;
    VkPipelineLayout                             _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo        _depthStencil;
    VkPipeline                                   build_pipeline(VkDevice device, VkPipelineRenderingCreateInfoKHR renderInfo);
};

struct FrameData {
    VkSemaphore                 c_swapchainSemphore, c_renderSemphore;
    VkFence                     c_renderFence;
    DescriptorAllocatorGrowable c_frameDescriptors;

    VkCommandPool   c_cmdPool;
    VkCommandBuffer c_mainCmd;
};

class VulkanEngine {
  public:
    vkb::DispatchTable deviceFunctions;
    bool               _isInitialized{false};
    int                _frameNumber{0};
    int                _selectedShader{0};

    VkExtent2D _windowExtent{1700, 900};

    struct SDL_Window *_window{nullptr};

    VkInstance               _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice         _chosenGPU;
    VkDevice                 _device;

    VkPhysicalDeviceProperties _gpuProperties;

    VkQueue  _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkQueue  _transferQueue;
    uint32_t _transferQueueFamily;

    VkSurfaceKHR   _surface;
    VkSwapchainKHR _swapchain;
    VkFormat       _swapchainImageFormat;

    std::vector<VkImage>     _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    std::vector<AllocatedImage> hdrimages;
    std::vector<VkImageView>    hdrImageViews;

    VmaAllocator _allocator; // vma lib allocator

    // depth resources
    VkImageView    _depthImageView;
    AllocatedImage _depthImage;

    VkImageView    colorImageView;
    AllocatedImage colorImage;

    VkFormat _depthFormat;

    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkDescriptorSetLayout _singleTextureSetLayout;

    AllocatedBuffer _sceneParameterBuffer;

    VkDescriptorPool _imgui_pool;

    Camera _cam;

    VkSampler _blockySampler;
    VkSampler hdrSampler;

    VkCommandBuffer cmd;
    VkCommandPool   pool;

    VkPipeline       pipeline;
    VkPipelineLayout pipelineLayout;

    VkPipeline       hdrPipeline;
    VkPipelineLayout hdrLayout;

    VkFence fence_wait;

    VkSemaphore     present_semp;
    VkSemaphore     render_semp;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    VkSampler _blockSampler;

    AllocatedImage _cubemap;
    VkImageView    _cubeview;

    std::vector<FrameData>      c_frames;
    DescriptorAllocatorGrowable c_globalAllocator;

    VkDescriptorSetLayout c_objectLayout;
    VkDescriptorSetLayout c_cameraLayout;
    VkDescriptorSetLayout c_textureLayout;

    VkDescriptorSet c_textureSet;

    AllocatedBuffer c_buffer;

    void init();

    // shuts down the engine
    void cleanup();

    void draw_test();

    // draw loop
    void draw();

    // run main loop
    void run();

  private:
    void init_vulkan();

    void init_swapchain();

    void init_commands();

    void init_sync_structures();

    void init_pipelines(std::unordered_map<std::string, VkShaderModule> &shaders);

    void init_imgui();

    void init_scene();

    void init_descriptors();

    void init_hdr();

    void create_vertex_buffer();
    /*Helper functions*/

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};
