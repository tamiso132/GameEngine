// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "common/std_common.h"

#include "../camera/camera.h"
#include "VkBootstrapDispatch.h"
#include "util/pipeline.h"
#include "util/vk_types.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "object.h"

struct FrameData {
    VkFence     queueDoneFence;
    VkSemaphore presentReady;
    VkSemaphore imageAquired;

    VkCommandPool   pool;
    VkCommandBuffer cmd;

    // First is Object, Second is Camera
    ResourceIndex dynamicIndex;

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

    VkDescriptorPool _imgui_pool;

    Camera _cam;

    ResourceContext ci_resourceContext = {};

    Mesh ci_meshes;

    Pipeline        pipeline;
    PipelineContext ci_mainPipeline;

    VkDescriptorSetLayout ci_textureLayout;
    VkDescriptorSetLayout ci_dynamicLayout;

    uint64_t ci_currentFrame = 0;
    void     init();

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
