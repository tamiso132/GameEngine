// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include "mesh.h"

#include "../camera/camera.h"
#include "common/std_common.h"
#include "util/helper.h"
#include "vk_types.h"
#include <VkBootstrap.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class ShaderContext {
  public:
    void init_all_shaders(VkDevice device) {
        std::filesystem::path shader_path = PROJECT_ROOT_PATH + std::string("/shaders");

        // Iterate over the files in the folder
        for (const auto &entry : std::filesystem::directory_iterator(shader_path)) {
            VkShaderModule module;
            util::load_shader_module(device, entry.path().c_str(), &module);
            ci_shaders[entry.path().filename()] = module;
        }
    }
    std::unordered_map<std::string, VkShaderModule> ci_shaders;
};


struct VulkanContextGlobal {
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice         chosenGPU;
    VkDevice                 device;
    vkb::DispatchTable       deviceFunctions;
    VkExtent2D               windowExtent{1700, 900};
    struct SDL_Window       *window{nullptr};
};
class VulkanEngine {
  public:
    bool                _isInitialized{false};
    VulkanContextGlobal ci_vulkanGlobal;

    VkPhysicalDeviceProperties _gpuProperties;

    VkQueue  _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkQueue  _transferQueue;
    uint32_t _transferQueueFamily;

    VkSurfaceKHR   _surface;
    VkSwapchainKHR _swapchain;
    VkFormat       _swapchainImageFormat;

    std::vector<VkImage>     ci_swapchainImages;
    std::vector<VkImageView> ci_swapchainImageViews;

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

    VkDescriptorSetLayout c_objectLayout;
    VkDescriptorSetLayout c_cameraLayout;
    VkDescriptorSetLayout c_textureLayout;

    VkDescriptorSet c_textureSet;

    AllocatedBuffer c_buffer;

    ResourceContext ci_resourceContext;

    ShaderContext ci_shaderContext;

    Mesh ci_meshes;

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
