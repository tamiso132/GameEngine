// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "VkBootstrapDispatch.h"
#include "mesh.h"

#include "../camera/camera.h"
#include "common/std_common.h"
#include "pipeline.h"
#include "vk_resource.h"
#include "vk_types.h"
#include <VkBootstrap.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

struct FrameData {
    VkFence     queueDoneFence;
    VkSemaphore presentReady;
    VkSemaphore imageAquired;

    VkCommandPool   pool;
    VkCommandBuffer cmd;

    DescriptorAllocatorGrowable descriptorAllocator;
};

class VulkanEngine {
  public:
    bool                _isInitialized{false};
    VulkanContextGlobal ci_vulkanGlobal = {};

    DescriptorAllocatorGrowable ci_globalDescriptorAllocator;

    std::vector<FrameData> ci_frames;

    VkQueue  _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkQueue  _transferQueue;
    uint32_t _transferQueueFamily;

    VkSurfaceKHR   _surface;
    VkSwapchainKHR _swapchain;
    VkFormat       _swapchainImageFormat;

    std::vector<VkImage>     ci_swapchainImages;
    std::vector<VkImageView> ci_swapchainImageViews;

    VmaAllocator _allocator; // vma lib allocator

    // depth resources
    VkImageView    _depthImageView;
    AllocatedImage _depthImage;

    VkFormat _depthFormat;

    VkDescriptorPool _imgui_pool;

    Camera _cam;

    VkCommandBuffer cmd;
    VkCommandPool   pool;

    VkDescriptorSet c_textureSet;

    ResourceContext ci_resourceContext = {};

    Mesh ci_meshes;

    Pipeline pipeline;

    uint64_t ci_currentFrame = 0;

    void init();

    // shuts down the engine
    void cleanup();

    void draw_test();

    // draw loop
    void draw();

    // run main loop
    void run();

  private:
    void init_vulkan(vkb::DispatchTable &table);

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
