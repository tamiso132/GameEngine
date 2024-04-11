// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "VkBootstrapDispatch.h"
#include "util/mesh.h"

#include "../camera/camera.h"
#include "common/std_common.h"
#include "util/pipeline.h"
#include "util/vk_resource.h"
#include "util/vk_types.h"
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

    ResourceIndex bufferIndex;

    DescriptorAllocatorGrowable descriptorAllocator;
};

class VulkanEngine {
  public:
    bool                _isInitialized{false};
    VulkanContextGlobal ci_vulkanGlobal = {};

    DescriptorAllocatorGrowable ci_globalDescriptorAllocator;

    std::vector<FrameData> ci_frames;

    VmaAllocator _allocator; // vma lib allocator

    // depth resources
    VkImageView    _depthImageView;
    AllocatedImage _depthImage;

    VkFormat _depthFormat;

    VkDescriptorPool _imgui_pool;

    Camera _cam;

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
