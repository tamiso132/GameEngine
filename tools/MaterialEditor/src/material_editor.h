// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "common/common_types.h"

#include "../camera/camera.h"
#include "VkBootstrapDispatch.h"
#include "util/pipeline.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "gui/gui.h"

struct TextureAtlasImage {
    const char *name; // TODO, get a thumbnail
};

struct TextureAtlas {
    uint8_t     id;
    uint8_t     pixelSize;
    uint16_t    width;
    uint16_t    height;
    const char *name;

    std::vector<TextureAtlasImage> textureImages;
};

struct FrameData {
    VkFence     queueDoneFence;
    VkSemaphore presentReady;
    VkSemaphore imageAquired;

    VkCommandPool   pool;
    VkCommandBuffer cmd;

    ResourceIndex bufferIndex;

    DescriptorAllocatorGrowable descriptorAllocator;
};

class MaterialEditor {
  public:
    bool                _isInitialized{false};
    VulkanContextGlobal ci_vulkanGlobal = {};

    DescriptorAllocatorGrowable ci_globalDescriptorAllocator;

    VkDescriptorPool ci_imguiPool;

    std::vector<FrameData> ci_frames;

    VmaAllocator _allocator; // vma lib allocator

    // depth resources

    Camera _cam;

    ResourceContext ci_resourceContext = {};

    Mesh ci_meshes;

    Pipeline pipeline;

    VkDescriptorSetLayout ci_textureLayout;

    uint64_t ci_currentFrame = 0;

    TGui gui;

    void init();
    // shuts down the engine
    void cleanup();

    void draw_test();

    // draw loop
    void draw();

    // run main loop
    void run();
};
