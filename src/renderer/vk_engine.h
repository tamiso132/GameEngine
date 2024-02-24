// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <glm/fwd.hpp>
#define GLM_ENABLE_EXPERIMENTAL

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <deque>
#include <functional>
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

struct alignas(16) GPUObject {
    glm::mat4 transformMatrix;
};

struct alignas(16) GPUCamera {
    glm::mat4 viewproj;
    glm::vec3 camPos;
};

struct alignas(16) GPUMaterial {
    glm::vec3 ambient;
    glm::float32 shininess;
    glm::vec3 diffuse;
    char padding2[4];
    glm::vec3 specular;
};

struct alignas(16) GPUAlignedArrayElement {
    uint32_t faceIndex;
};

struct alignas(16) GPUTexture {
    GPUAlignedArrayElement faceIndices[6]; // 12-byte padding
    GPUMaterial material;
};

struct Texture {
    AllocatedImage image;
    VkImageView imageView;
};

// G-buffer attachement
struct FrameBufferAttachment {
    AllocatedImage image;
    VkImageView view;
    VkFormat format;
};
struct Attachments {
    FrameBufferAttachment position, normal, albedo;
    int32_t width;
    int32_t height;
};

class PipelineBuilder {
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkViewport _viewport;
    VkRect2D _scissor;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

class VulkanEngine {
  public:
    bool _isInitialized{false};
    int _frameNumber{0};
    int _selectedShader{0};

    VkExtent2D _windowExtent{1700, 900};

    struct SDL_Window *_window{nullptr};

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _chosenGPU;
    VkDevice _device;

    VkPhysicalDeviceProperties _gpuProperties;

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

    VkRenderPass _renderPass;
    VkRenderPass brightRenderPass;
    VkRenderPass blurRenderPass;

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    VkFormat _swachainImageFormat;

    std::vector<VkFramebuffer> _framebuffers;
    std::vector<VkFramebuffer> brightnessFrameBuffer;
    std::vector<VkFramebuffer> blurFrameBuffer;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VmaAllocator _allocator; // vma lib allocator

    // depth resources
    VkImageView _depthImageView;
    AllocatedImage _depthImage;

    VkImageView colorImageView;
    AllocatedImage colorImage;

    VkFormat _depthFormat;

    VkDescriptorSetLayout _globalSetLayout;
    VkDescriptorSetLayout _objectSetLayout;
    VkDescriptorSetLayout _singleTextureSetLayout;

    VkDescriptorSetLayout _objectOpenglLayout;

    AllocatedBuffer _sceneParameterBuffer;

    VkDescriptorPool _imgui_pool;

    vkutil::DescriptorAllocator *_descriptorAllocator;
    vkutil::DescriptorLayoutCache *_descriptorLayoutCache;

    Camera _cam;

    VkPipeline _opengl_pipeline;
    VkPipelineLayout _opengl_layout;

    VkSampler _blockySampler;

    std::vector<VertexOpengl> _openglVertices;

    GlobalState global;

    VkCommandBuffer cmd;
    VkCommandPool pool;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkFence fence_wait;

    VkSemaphore present_semp;
    VkSemaphore render_semp;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    VkSampler _blockSampler;

    AllocatedImage _cubemap;
    VkImageView _cubeview;

    AllocatedImage _normalMap;
    VkImageView _normalView;

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

    void init_default_renderpass();

    void init_framebuffers();

    void init_commands();

    void init_sync_structures();

    void init_pipelines(std::unordered_map<std::string, VkShaderModule> &shaders);

    void init_imgui();

    void init_scene();

    void init_descriptors();

    void create_vertex_buffer();
    /*Helper functions*/

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};
