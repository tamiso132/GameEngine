// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>
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

#include "camera/camera.h"
#include "vk_descriptors.h"
#include "vk_mesh.h"
#include "vk_types.h"

#include "vk_create.h"

struct VertexTemp {
  // probably want texture
  // positions
  // normal
  // uv
};

struct MyObject {
  glm::mat4 transform_matrix;
  std::vector<VertexTemp> vertices;
};

struct GPUObject {
  glm::mat4 transformMatrix;
};
struct GPUCamera {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
};

struct ShaderLayout {
  std::vector<uint> shader_indexes;
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

constexpr unsigned int FRAME_OVERLAP = 2;

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

  // FrameData _frames[FRAME_OVERLAP];

  // FrameDataOpengl _openglFrames[FRAME_OVERLAP];

  VkQueue _graphicsQueue;
  uint32_t _graphicsQueueFamily;

  VkRenderPass _renderPass;

  VkSurfaceKHR _surface;
  VkSwapchainKHR _swapchain;
  VkFormat _swachainImageFormat;

  std::vector<VkFramebuffer> _framebuffers;
  std::vector<VkImage> _swapchainImages;
  std::vector<VkImageView> _swapchainImageViews;

  // DeletionQueue _mainDeletionQueue;

  VmaAllocator _allocator; // vma lib allocator

  // depth resources
  VkImageView _depthImageView;
  AllocatedImage _depthImage;

  // the format for the depth image
  VkFormat _depthFormat;

  VkDescriptorSetLayout _globalSetLayout;
  VkDescriptorSetLayout _objectSetLayout;
  VkDescriptorSetLayout _singleTextureSetLayout;

  VkDescriptorSetLayout _objectOpenglLayout;

  // GPUSceneData _sceneParameters;
  AllocatedBuffer _sceneParameterBuffer;

  // UploadContext _uploadContext;
  //  initializes everything in the engine

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

  void init();

  // shuts down the engine
  void cleanup();

  // draw loop
  void draw();

  // run main loop
  void run();

  AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage,
                                VmaMemoryUsage memoryUsage);

  size_t pad_uniform_buffer_size(size_t originalSize);

  void immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function);

private:
  void init_vulkan();

  void init_swapchain();

  void init_default_renderpass();

  void init_framebuffers();

  void init_commands();

  void init_sync_structures();

  void init_pipelines(std::unordered_map<const char *, VkShaderModule> shaders);

  void init_imgui();

  void init_scene();

  void init_descriptors();

  bool load_shader_module(const char *filePath,
                          VkShaderModule *outShaderModule);

  void load_meshes();

  void load_images();

  void upload_mesh(Mesh &mesh);

  void update_material_pipeline(VkPipeline pipeline, VkPipelineLayout layout,
                                const std::string &name);
};
