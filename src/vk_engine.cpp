
#include "vk_engine.h"

#include <imgui.h>

#include <fstream>
#include <iostream>

#include "VkBootstrap.h"
#include "vk_initializers.h"
#include "vk_textures.h"
#include "vk_types.h"

#define VMA_IMPLEMENTATION
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vk_descriptors.h"
#include "vk_mem_alloc.h"

constexpr bool bUseValidationLayers = true;

// we want to immediately abort when there is an error. In normal engines this
// would give an error message to the user, or perform a dump of state.
using namespace std;

std::vector<const char *> device_extensions = {"VK_KHR_dynamic_rendering"};

void VulkanEngine::init() {
  // We initialize SDL and create a window with it.
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  _window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowExtent.width,
                             _windowExtent.height, window_flags);

  init_vulkan();

  init_swapchain();

  init_default_renderpass();

  init_framebuffers();

  init_commands();

  init_sync_structures();

  load_images();

  load_meshes();

  init_descriptors();

  init_pipelines();

  init_imgui();

  init_scene();

  SDL_Rect rect;
  SDL_GetDisplayBounds(1, &rect);
  SDL_SetWindowPosition(_window, rect.x, rect.y);
  // everything went fine
  _isInitialized = true;
}
void VulkanEngine::cleanup() {
  if (_isInitialized) {
    // make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(_device);

    _mainDeletionQueue.flush();

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroySampler(_device, _blockySampler, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);

    SDL_DestroyWindow(_window);
  }
}

void VulkanEngine::draw() {
  // check if window is minimized and skip drawing
  if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) return;
  /*SYNC FRAME*/
  // wait until the gpu has finished rendering the last frame. Timeout of 1
  // second
  VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
  VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

  // now that we are sure that the commands finished executing, we can safely
  // reset the command buffer to begin recording again.
  VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

  // request image from the swapchain
  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr,
                                 &swapchainImageIndex));

  /*RENDERING*/
  ImGui::Render();
  // naming it cmd for shorter writing
  VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

  // begin the command buffer recording.We will use this command buffer exactly
  // once, so we want to let vulkan know that
  VkCommandBufferBeginInfo cmdBeginInfo =
      vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  // make a clear-color from frame number. This will flash with a 120 frame
  // period.
  VkClearValue clearValue;
  float flash = abs(sin(_frameNumber / 120.f));
  clearValue.color = {{0.0f, 0.0f, flash, 1.0f}};

  // clear depth at 1
  VkClearValue depthClear;
  depthClear.depthStencil.depth = 1.f;

  // start the main renderpass.
  // We will use the clear color from above, and the framebuffer of the index
  // the swapchain gave us
  VkRenderPassBeginInfo rpInfo =
      vkinit::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);

  // connect clear values
  rpInfo.clearValueCount = 2;

  VkClearValue clearValues[] = {clearValue, depthClear};

  rpInfo.pClearValues = &clearValues[0];

  vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  draw_objects(cmd, _renderables.data(), _renderables.size());

  // draw imgui
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  // finalize the render pass
  vkCmdEndRenderPass(cmd);
  // finalize the command buffer (we can no longer add commands, but it can now
  // be executed)

  /*DYNAMIC RENDERING*/
  /////

  // const VkRenderingAttachmentInfoKHR color_attachment_info{
  //     .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
  //     .imageView = _swapchainImageViews[swapchainImageIndex],
  //     .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
  //     .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
  //     .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
  //     .clearValue = clear_value,
  // };

  // const VkRenderingInfoKHR render_info{
  //     .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
  //     .renderArea = _meshpip,
  //     .layerCount = 1,
  //     .colorAttachmentCount = 1,
  //     .pColorAttachments = &color_attachment_info,
  // };

  // vkCmdBeginRenderingKHR(command_buffer, &render_info);

  // // Draw calls here

  // vkCmdEndRenderingKHR(command_buffer);

  VK_CHECK(vkEndCommandBuffer(cmd));

  // prepare the submission to the queue.
  // we want to wait on the _presentSemaphore, as that semaphore is signaled
  // when the swapchain is ready we will signal the _renderSemaphore, to signal
  // that rendering has finished

  VkSubmitInfo submit = vkinit::submit_info(&cmd);
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  submit.pWaitDstStageMask = &waitStage;

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;

  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

  // prepare present
  //  this will put the image we just rendered to into the visible window.
  //  we want to wait on the _renderSemaphore for that,
  //  as its necessary that drawing commands have finished before the image is
  //  displayed to the user
  VkPresentInfoKHR presentInfo = vkinit::present_info();

  presentInfo.pSwapchains = &_swapchain;
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;

  VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

  // increase the number of frames drawn
  _frameNumber++;
}

void VulkanEngine::run() {
  SDL_Event e;
  bool bQuit = false;

  // Initialize time variables

  // Define frame rate and calculate frame duration
  const double target_fps = 60.0;
  const double target_frame_duration = 1.0 / target_fps;

  float delta_time = 0;
  auto last_frame = 0.0f;

  SDL_SetRelativeMouseMode(SDL_TRUE);
  SDL_bool mouse_inside_window = SDL_TRUE;

  int relX, relY;
  bool move_keys[] = {0, 0, 0, 0};
  // main loop
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  while (!bQuit) {
    float current_frame = (float)SDL_GetTicks64() / 1000;

    delta_time = current_frame - last_frame;
    last_frame = current_frame;

    SDL_GetRelativeMouseState(&relX, &relY);
    _cam.process_input(nullptr, delta_time, relX, relY, keystate);
    while (SDL_PollEvent(&e) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT) {
        bQuit = true;
      }
      if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          mouse_inside_window = (SDL_bool)!mouse_inside_window;
          SDL_SetRelativeMouseMode(mouse_inside_window);
        }
      }
    }
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);

    ImGui::NewFrame();

    // imgui commands
    ImGui::ShowDemoWindow();
    draw();
  }
}

FrameData &VulkanEngine::get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

FrameData &VulkanEngine::get_last_frame() { return _frames[(_frameNumber - 1) % 2]; }

void VulkanEngine::init_vulkan() {
  vkb::InstanceBuilder builder;

  // make the vulkan instance, with basic debug features
  auto inst_ret = builder.set_app_name("Example Vulkan Application")
                      .request_validation_layers(bUseValidationLayers)
                      .use_default_debug_messenger()
                      .require_api_version(1, 3, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();

  // grab the instance
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
  // use vkbootstrap to select a gpu.
  // We want a gpu that can write to the SDL surface and supports vulkan 1.2
  VkPhysicalDeviceVulkan11Features features_11;
  features_11.shaderDrawParameters = true;

  VkPhysicalDeviceVulkan13Features features_13;
  features_13.dynamicRendering = true;

  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                           .set_required_features_11(features_11)
                                           .set_required_features_13(features_13)
                                           .add_required_extension(device_extensions[0])
                                           .set_surface(_surface)
                                           .select()
                                           .value();

  // create the final vulkan device

  vkb::DeviceBuilder deviceBuilder{physicalDevice};

  vkb::Device vkbDevice = deviceBuilder.build().value();

  // Get the VkDevice handle used in the rest of a vulkan application
  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;

  // use vkbootstrap to get a Graphics queue
  _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();

  _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

  // initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _mainDeletionQueue.push_function([&]() { vmaDestroyAllocator(_allocator); });

  vkGetPhysicalDeviceProperties(_chosenGPU, &_gpuProperties);

  std::cout << "The gpu has a minimum buffer alignement of " << _gpuProperties.limits.minUniformBufferOffsetAlignment
            << std::endl;
}

void VulkanEngine::init_swapchain() {
  vkb::SwapchainBuilder swapchainBuilder{_chosenGPU, _device, _surface};

  vkb::Swapchain vkbSwapchain = swapchainBuilder
                                    .use_default_format_selection()
                                    // use vsync present mode
                                    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                    .set_desired_extent(_windowExtent.width, _windowExtent.height)
                                    .build()
                                    .value();

  // store swapchain and its related images
  _swapchain = vkbSwapchain.swapchain;
  _swapchainImages = vkbSwapchain.get_images().value();
  _swapchainImageViews = vkbSwapchain.get_image_views().value();

  _swachainImageFormat = vkbSwapchain.image_format;

  _mainDeletionQueue.push_function([=]() { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });

  // depth image size will match the window
  VkExtent3D depthImageExtent = {_windowExtent.width, _windowExtent.height, 1};

  // hardcoding the depth format to 32 bit float
  _depthFormat = VK_FORMAT_D32_SFLOAT;

  // the depth image will be a image with the format we selected and Depth
  // Attachment usage flag
  VkImageCreateInfo dimg_info =
      vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

  // for the depth image, we want to allocate it from gpu local memory
  VmaAllocationCreateInfo dimg_allocinfo = {};
  dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate and create the image
  vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

  // build a image-view for the depth image to use for rendering
  VkImageViewCreateInfo dview_info =
      vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);
  ;

  VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));

  // add to deletion queues
  _mainDeletionQueue.push_function([=]() {
    vkDestroyImageView(_device, _depthImageView, nullptr);
    vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
  });
}

void VulkanEngine::init_default_renderpass() {
  // we define an attachment description for our main color image
  // the attachment is loaded as "clear" when renderpass start
  // the attachment is stored when renderpass ends
  // the attachment layout starts as "undefined", and transitions to "Present"
  // so its possible to display it we dont care about stencil, and dont use
  // multisampling

  VkAttachmentDescription color_attachment = {};
  color_attachment.format = _swachainImageFormat;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depth_attachment = {};
  // Depth attachment
  depth_attachment.flags = 0;
  depth_attachment.format = _depthFormat;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  // we are going to create 1 subpass, which is the minimum you can do
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  // hook the depth attachment into the subpass
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  // 1 dependency, which is from "outside" into the subpass. And we can read or
  // write color
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // dependency from outside to the subpass, making this subpass dependent on
  // the previous renderpasses
  VkSubpassDependency depth_dependency = {};
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  // array of 2 dependencies, one for color, two for depth
  VkSubpassDependency dependencies[2] = {dependency, depth_dependency};

  // array of 2 attachments, one for the color, and other for depth
  VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  // 2 attachments from attachment array
  render_pass_info.attachmentCount = 2;
  render_pass_info.pAttachments = &attachments[0];
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  // 2 dependencies from dependency array
  render_pass_info.dependencyCount = 2;
  render_pass_info.pDependencies = &dependencies[0];

  VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

  _mainDeletionQueue.push_function([=]() { vkDestroyRenderPass(_device, _renderPass, nullptr); });
}

void VulkanEngine::init_framebuffers() {
  // create the framebuffers for the swapchain images. This will connect the
  // render-pass to the images for rendering
  VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_renderPass, _windowExtent);

  const uint32_t swapchain_imagecount = _swapchainImages.size();
  _framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

  for (int i = 0; i < swapchain_imagecount; i++) {
    VkImageView attachments[2];
    attachments[0] = _swapchainImageViews[i];
    attachments[1] = _depthImageView;

    fb_info.pAttachments = attachments;
    fb_info.attachmentCount = 2;
    VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

    _mainDeletionQueue.push_function([=]() {
      vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
      vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    });
  }
}

void VulkanEngine::init_commands() {
  // create a command pool for commands submitted to the graphics queue.
  // we also want the pool to allow for resetting of individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo =
      vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

    // allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

    _mainDeletionQueue.push_function([=]() { vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr); });
  }

  VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);
  // create pool for upload context
  VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));

  _mainDeletionQueue.push_function([=]() { vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr); });

  // allocate the default command buffer that we will use for rendering
  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);

  VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer));
}

void VulkanEngine::init_sync_structures() {
  // create syncronization structures
  // one fence to control when the gpu has finished rendering the frame,
  // and 2 semaphores to syncronize rendering with swapchain
  // we want the fence to start signalled so we can wait on it on the first
  // frame
  VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

    // enqueue the destruction of the fence
    _mainDeletionQueue.push_function([=]() { vkDestroyFence(_device, _frames[i]._renderFence, nullptr); });

    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
    VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

    // enqueue the destruction of semaphores
    _mainDeletionQueue.push_function([=]() {
      vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
      vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
    });
  }

  VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();

  VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
  _mainDeletionQueue.push_function([=]() { vkDestroyFence(_device, _uploadContext._uploadFence, nullptr); });
}

void VulkanEngine::init_pipelines() {
  /*SHADER MODULE*/

  VkShaderModule colorMeshShader;
  if (!load_shader_module("shaders/spiv/default_lit.frag.spv", &colorMeshShader)) {
    std::cout << "Error when building the colored mesh shader" << std::endl;
  }

  VkShaderModule texturedMeshShader;
  if (!load_shader_module("shaders/spiv/textured_lit.frag.spv", &texturedMeshShader)) {
    std::cout << "Error when building the colored mesh shader" << std::endl;
  }

  VkShaderModule meshVertShader;
  if (!load_shader_module("shaders/spiv/tri_mesh_ssbo.vert.spv", &meshVertShader)) {
    std::cout << "Error when building the mesh vertex shader module" << std::endl;
  }

  VkShaderModule triangle_shader_frag;
  VkShaderModule triangle_shader_vert;

  if (!load_shader_module("shaders/spiv/funky_triangle.frag.spv", &triangle_shader_frag)) {
    std::cout << "Error when building the mesh vertex shader module" << std::endl;
  }
  if (!load_shader_module("shaders/spiv/colored_triangle.vert.spv", &triangle_shader_vert)) {
    std::cout << "Error when building the mesh vertex shader module" << std::endl;
  }

  // build the stage-create-info for both vertex and fragment stages. This lets
  // the pipeline know the shader modules per stage
  PipelineBuilder pipelineBuilder;

  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, colorMeshShader));

  /*PIPELINE LAYOUT*/
  // we start from just the default empty pipeline layout info
  VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();

  VkPipelineLayoutCreateInfo test_pipeline_layout_info = vkinit::pipeline_layout_create_info();

  // setup push constants
  VkPushConstantRange push_constant;
  // offset 0
  push_constant.offset = 0;
  // size of a MeshPushConstant struct
  push_constant.size = sizeof(MeshPushConstants);
  // for the vertex shader
  push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
  mesh_pipeline_layout_info.pushConstantRangeCount = 1;

  VkDescriptorSetLayout setLayouts[] = {_globalSetLayout, _objectSetLayout};
  VkDescriptorSetLayout setLayoutsOpenGl[] = {_globalSetLayout, _objectOpenglLayout};

  mesh_pipeline_layout_info.setLayoutCount = 2;
  mesh_pipeline_layout_info.pSetLayouts = setLayouts;

  test_pipeline_layout_info.setLayoutCount = 2;
  test_pipeline_layout_info.pSetLayouts = setLayoutsOpenGl;

  VkPipelineLayout meshPipLayout;
  VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &meshPipLayout));

  VK_CHECK(vkCreatePipelineLayout(_device, &test_pipeline_layout_info, nullptr, &_opengl_layout));

  // we start from  the normal mesh layout
  VkPipelineLayoutCreateInfo textured_pipeline_layout_info = mesh_pipeline_layout_info;

  VkDescriptorSetLayout texturedSetLayouts[] = {_globalSetLayout, _objectSetLayout, _singleTextureSetLayout};

  textured_pipeline_layout_info.setLayoutCount = 3;
  textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;

  VkPipelineLayout texturedPipeLayout;
  VK_CHECK(vkCreatePipelineLayout(_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout));

  /*VERTEX*/
  // hook the push constants layout
  pipelineBuilder._pipelineLayout = meshPipLayout;

  // vertex input controls how to read vertices from vertex buffers. We arent
  // using it yet
  pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();

  // input assembly is the configuration for drawing triangle lists, strips, or
  // individual points. we are just going to draw triangle list
  pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

  /*VIEWPORT*/
  // build viewport and scissor from the swapchain extents
  pipelineBuilder._viewport.x = 0.0f;
  pipelineBuilder._viewport.y = 0.0f;
  pipelineBuilder._viewport.width = (float)_windowExtent.width;
  pipelineBuilder._viewport.height = (float)_windowExtent.height;
  pipelineBuilder._viewport.minDepth = 0.0f;
  pipelineBuilder._viewport.maxDepth = 1.0f;

  pipelineBuilder._scissor.offset = {0, 0};
  pipelineBuilder._scissor.extent = _windowExtent;

  /*Extra*/
  // configure the rasterizer to draw filled triangles
  pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

  // we dont use multisampling, so just run the default one
  pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

  // a single blend attachment with no blending and writing to RGBA
  pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

  // default depthtesting
  pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

  /*Vertex Bindings*/

  VertexInputDescription vertexDescription = Vertex::get_vertex_description();

  pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
  pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

  pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
  pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

  VkPipeline meshPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

  create_material(meshPipeline, meshPipLayout, "defaultmesh");

  pipelineBuilder._shaderStages.clear();
  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));

  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, texturedMeshShader));

  pipelineBuilder._pipelineLayout = texturedPipeLayout;
  VkPipeline texPipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
  update_material_pipeline(texPipeline, texturedPipeLayout, "texturedmesh");

  pipelineBuilder._shaderStages.clear();

  /*OPENGL PIPELINE*/
  VertexInputDescription opengl_vertex_desc = VertexOpengl::get_vertex_description();

  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangle_shader_vert));
  pipelineBuilder._shaderStages.push_back(
      vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangle_shader_frag));
  //////////////// TRIANGLE
  ////////////////
  pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(false, false, VK_COMPARE_OP_ALWAYS);
  pipelineBuilder._pipelineLayout = _opengl_layout;

  /*VERTEX BINDINGS*/
  pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = opengl_vertex_desc.attributes.data();
  pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = opengl_vertex_desc.attributes.size();

  pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = opengl_vertex_desc.bindings.data();
  pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = opengl_vertex_desc.bindings.size();

  _opengl_pipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

  vkDestroyShaderModule(_device, meshVertShader, nullptr);
  vkDestroyShaderModule(_device, colorMeshShader, nullptr);
  vkDestroyShaderModule(_device, texturedMeshShader, nullptr);
  vkDestroyShaderModule(_device, triangle_shader_frag, nullptr);
  vkDestroyShaderModule(_device, triangle_shader_vert, nullptr);

  _mainDeletionQueue.push_function([=]() {
    vkDestroyPipeline(_device, meshPipeline, nullptr);
    vkDestroyPipeline(_device, texPipeline, nullptr);
    vkDestroyPipeline(_device, _opengl_pipeline, nullptr);

    vkDestroyPipelineLayout(_device, meshPipLayout, nullptr);
    vkDestroyPipelineLayout(_device, texturedPipeLayout, nullptr);
    vkDestroyPipelineLayout(_device, _opengl_layout, nullptr);
  });
}

void VulkanEngine::init_imgui() {
  // 1: create descriptor pool for IMGUI
  //  the size of the pool is very oversize, but it's copied from imgui demo
  //  itself.
  VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                       {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                       {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                       {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imgui_pool));

  // 2: initialize imgui library

  // this initializes the core structures of imgui
  ImGui::CreateContext();

  // this initializes imgui for SDL
  ImGui_ImplSDL2_InitForVulkan(_window);

  // this initializes imgui for Vulkan
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = _instance;
  init_info.PhysicalDevice = _chosenGPU;
  init_info.Device = _device;
  init_info.Queue = _graphicsQueue;
  init_info.DescriptorPool = _imgui_pool;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info, _renderPass);

  // execute a gpu command to upload imgui font textures
  immediate_submit([&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(); });

  // clear font textures from cpu data
  ImGui_ImplVulkan_DestroyFontsTexture();

  // add the destroy the imgui created structures
  _mainDeletionQueue.push_function([=]() {
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(_device, _imgui_pool, nullptr);
  });
}

bool VulkanEngine::load_shader_module(const char *filePath, VkShaderModule *outShaderModule) {
  std::string full_path = string(PROJECT_ROOT_PATH) + "/" + filePath;

  // open the file. With cursor at the end
  std::ifstream file(full_path, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  // find what the size of the file is by looking up the location of the cursor
  // because the cursor is at the end, it gives the size directly in bytes
  size_t fileSize = (size_t)file.tellg();

  // spirv expects the buffer to be on uint32, so make sure to reserve a int
  // vector big enough for the entire file
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  // put file cursor at beggining
  file.seekg(0);

  // load the entire file into the buffer
  file.read((char *)buffer.data(), fileSize);

  // now that the file is loaded into the buffer, we can close it
  file.close();

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  // codeSize has to be in bytes, so multply the ints in the buffer by size of
  // int to know the real size of the buffer
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  // check that the creation goes well.
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    return false;
  }
  *outShaderModule = shaderModule;
  return true;
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
  // make viewport state from our stored viewport and scissor.
  // at the moment we wont support multiple viewports or scissors
  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;

  viewportState.viewportCount = 1;
  viewportState.pViewports = &_viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &_scissor;

  // setup dummy color blending. We arent using transparent objects yet
  // the blending is just "no blend", but we do write to the color attachment
  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.pNext = nullptr;

  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &_colorBlendAttachment;

  // build the actual pipeline
  // we now use all of the info structs we have been writing into into this one
  // to create the pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;

  pipelineInfo.stageCount = _shaderStages.size();
  pipelineInfo.pStages = _shaderStages.data();
  pipelineInfo.pVertexInputState = &_vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &_inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &_rasterizer;
  pipelineInfo.pMultisampleState = &_multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &_depthStencil;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = pass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  // its easy to error out on create graphics pipeline, so we handle it a bit
  // better than the common VK_CHECK case
  VkPipeline newPipeline;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
    std::cout << "failed to create pipline\n";
    return VK_NULL_HANDLE;  // failed to create graphics pipeline
  } else {
    return newPipeline;
  }
}

void VulkanEngine::load_meshes() {
  Mesh triMesh{};
  // make the array 3 vertices long
  triMesh._vertices.resize(3);

  // vertex positions
  triMesh._vertices[0].position = {1.f, 1.f, 0.0f};
  triMesh._vertices[1].position = {-1.f, 1.f, 0.0f};
  triMesh._vertices[2].position = {0.f, -1.f, 0.0f};

  // vertex colors, all green
  triMesh._vertices[0].color = {0.f, 1.f, 0.0f};  // pure green
  triMesh._vertices[1].color = {0.f, 1.f, 0.0f};  // pure green
  triMesh._vertices[2].color = {0.f, 1.f, 0.0f};  // pure green

  VertexOpengl open_gl[3];
  open_gl[0].position = {-0.5f, -0.5f, 0.0f};
  open_gl[0].position = {0.5f, -0.5f, 0.0f};
  open_gl[0].position = {0.0f, 0.5f, 0.0f};
  // we dont care about the vertex normals

  // load the monkey
  Mesh monkeyMesh{};
  monkeyMesh.load_from_obj("assets/monkey_smooth.obj");

  Mesh lostEmpire{};
  lostEmpire.load_from_obj("assets/lost_empire.obj");

  upload_mesh(triMesh);
  upload_mesh(monkeyMesh);
  upload_mesh(lostEmpire);

  _meshes["monkey"] = monkeyMesh;
  _meshes["triangle"] = triMesh;
  _meshes["empire"] = lostEmpire;
}

void VulkanEngine::load_images() {
  Texture lostEmpire;

  vkutil::load_image_from_file(*this, "assets/lost_empire-RGBA.png", lostEmpire.image);

  VkImageViewCreateInfo imageinfo =
      vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
  vkCreateImageView(_device, &imageinfo, nullptr, &lostEmpire.imageView);

  _mainDeletionQueue.push_function([=]() { vkDestroyImageView(_device, lostEmpire.imageView, nullptr); });

  _loadedTextures["empire_diffuse"] = lostEmpire;
}

void VulkanEngine::upload_mesh(Mesh &mesh) {
  const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);
  // allocate vertex buffer
  VkBufferCreateInfo stagingBufferInfo = {};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.pNext = nullptr;
  // this is the total size, in bytes, of the buffer we are allocating
  stagingBufferInfo.size = bufferSize;
  // this buffer is going to be used as a Vertex Buffer
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  // let the VMA library know that this data should be writeable by CPU, but
  // also readable by GPU
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  AllocatedBuffer stagingBuffer;

  // allocate the buffer@
  VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer,
                           &stagingBuffer._allocation, nullptr));

  // copy vertex data
  void *data;
  vmaMapMemory(_allocator, stagingBuffer._allocation, &data);

  memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));

  vmaUnmapMemory(_allocator, stagingBuffer._allocation);

  // allocate vertex buffer
  VkBufferCreateInfo vertexBufferInfo = {};
  vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vertexBufferInfo.pNext = nullptr;
  // this is the total size, in bytes, of the buffer we are allocating
  vertexBufferInfo.size = bufferSize;
  // this buffer is going to be used as a Vertex Buffer
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  // let the VMA library know that this data should be gpu native
  vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  // allocate the buffer
  VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaallocInfo, &mesh._vertexBuffer._buffer,
                           &mesh._vertexBuffer._allocation, nullptr));
  // add the destruction of triangle mesh buffer to the deletion queue
  _mainDeletionQueue.push_function(
      [=]() { vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation); });

  immediate_submit([=](VkCommandBuffer cmd) {
    VkBufferCopy copy;
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
  });

  vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

Material *VulkanEngine::create_material(const std::string &name) {
  Material mat;
  mat.pipeline = nullptr;
  mat.pipelineLayout = nullptr;
  _materials[name] = mat;
  return &_materials[name];
}

Material *VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name) {
  Material mat;
  mat.pipeline = pipeline;
  mat.pipelineLayout = layout;
  _materials[name] = mat;
  return &_materials[name];
}

void VulkanEngine::update_material_pipeline(VkPipeline pipeline, VkPipelineLayout layout, const std::string &name) {
  Material *mat = &_materials[name];
  mat->pipeline = pipeline;
  mat->pipelineLayout = layout;
}

Material *VulkanEngine::get_material(const std::string &name) {
  // search for the object, and return nullpointer if not found
  auto it = _materials.find(name);
  if (it == _materials.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

Mesh *VulkanEngine::get_mesh(const std::string &name) {
  auto it = _meshes.find(name);
  if (it == _meshes.end()) {
    return nullptr;
  } else {
    return &(*it).second;
  }
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject *first, int count) {
  // make a model view matrix for rendering the object
  // camera view

  /*Camera*/
  auto view = _cam.get_view();
  //  camera projection
  glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
  projection[1][1] *= -1;

  GPUCameraData camData;
  camData.proj = projection;
  camData.view = view;
  camData.viewproj = projection * camData.view;

  void *data;
  vmaMapMemory(_allocator, get_current_frame().cameraBuffer._allocation, &data);

  memcpy(data, &camData, sizeof(GPUCameraData));

  vmaUnmapMemory(_allocator, get_current_frame().cameraBuffer._allocation);

  /*Scene*/

  float framed = (_frameNumber / 120.f);
  _sceneParameters.ambientColor = {sin(framed), 0, cos(framed), 1};
  _sceneParameters.fogColor = {0.5, 0.5, 0.5, 1};
  _sceneParameters.fogDistances = {0, 1, 0, 0};

  char *sceneData;
  vmaMapMemory(_allocator, _sceneParameterBuffer._allocation, (void **)&sceneData);

  int frameIndex = _frameNumber % FRAME_OVERLAP;

  sceneData += pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;

  memcpy(sceneData, &_sceneParameters, sizeof(GPUSceneData));

  vmaUnmapMemory(_allocator, _sceneParameterBuffer._allocation);
  /*Object*/
  void *objectData;
  vmaMapMemory(_allocator, get_current_frame().objectBuffer._allocation, &objectData);

  GPUObjectData *objectSSBO = (GPUObjectData *)objectData;

  for (int i = 0; i < count; i++) {
    RenderObject &object = first[i];
    objectSSBO[i].modelMatrix = object.transformMatrix;
  }

  vmaUnmapMemory(_allocator, get_current_frame().objectBuffer._allocation);

  Mesh *lastMesh = nullptr;
  Material *lastMaterial = nullptr;

  for (int i = 0; i < count; i++) {
    RenderObject &object = first[i];

    // only bind the pipeline if it doesnt match with the already bound one
    if (object.material != lastMaterial) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
      lastMaterial = object.material;

      uint32_t uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData)) * frameIndex;
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1,
                              &get_current_frame().globalDescriptor, 1, &uniform_offset);

      // object data descriptor
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1,
                              &get_current_frame().objectDescriptor, 0, nullptr);

      if (object.material->textureSet != VK_NULL_HANDLE) {
        // texture descriptor
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1,
                                &object.material->textureSet, 0, nullptr);
      }
    }

    glm::mat4 model = object.transformMatrix;
    // final render matrix, that we are calculating on the cpu
    glm::mat4 mesh_matrix = model;

    MeshPushConstants constants;
    constants.render_matrix = mesh_matrix;

    // upload the mesh to the gpu via pushconstants
    vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
                       &constants);
    // only bind the mesh if its a different one from last bind
    if (object.mesh != lastMesh) {
      // bind the mesh vertex buffer with offset 0
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
      lastMesh = object.mesh;
    }
    // we can now draw
    vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, i);
  }

  /*OpenGL*/
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _opengl_pipeline);
  uint32_t uniform_offset = pad_uniform_buffer_size(sizeof(GPUSceneData));
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _opengl_layout, 0, 1,
                          &get_current_frame().globalDescriptor, 1, nullptr);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _opengl_layout, 0, 1,
                          &_openglFrames[0].objectDescriptor, 1, nullptr);

  auto mesh = get_mesh("openglMesh");

  vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->_vertexBuffer._buffer, 0);

  vkCmdDraw(cmd, mesh->_vertices.size(), 1, 0, 0);

  // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _opengl_pipeline);
  // vkCmdDraw(cmd, 3, 1, 0, 0);
}

void VulkanEngine::init_scene() {
  RenderObject monkey;
  monkey.mesh = get_mesh("monkey");
  monkey.material = get_material("defaultmesh");
  monkey.transformMatrix = glm::mat4{1.0f};

  //_renderables.push_back(monkey);

  RenderObject map;
  map.mesh = get_mesh("empire");
  map.material = get_material("texturedmesh");
  map.transformMatrix = glm::translate(glm::vec3{5, -10, 0});  // glm::mat4{ 1.0f };

  _renderables.push_back(map);

  for (int x = -20; x <= 20; x++) {
    for (int y = -20; y <= 20; y++) {
      RenderObject tri;
      tri.mesh = get_mesh("triangle");
      tri.material = get_material("defaultmesh");
      glm::mat4 translation = glm::translate(glm::mat4{1.0}, glm::vec3(x, 0, y));
      glm::mat4 scale = glm::scale(glm::mat4{1.0}, glm::vec3(0.2, 0.2, 0.2));
      tri.transformMatrix = translation * scale;

      _renderables.push_back(tri);
    }
  }
}

AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
  // allocate vertex buffer
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.size = allocSize;

  bufferInfo.usage = usage;

  // let the VMA library know that this data should be writeable by CPU, but
  // also readable by GPU
  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;

  AllocatedBuffer newBuffer;

  // allocate the buffer
  VK_CHECK(
      vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));

  return newBuffer;
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize) {
  // Calculate required alignment based on minimum device offset alignment
  size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
  size_t alignedSize = originalSize;
  if (minUboAlignment > 0) {
    alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
  }
  return alignedSize;
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)> &&function) {
  VkCommandBuffer cmd = _uploadContext._commandBuffer;
  // begin the command buffer recording. We will use this command buffer exactly
  // once, so we want to let vulkan know that
  VkCommandBufferBeginInfo cmdBeginInfo =
      vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  function(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkSubmitInfo submit = vkinit::submit_info(&cmd);

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

  vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
  vkResetFences(_device, 1, &_uploadContext._uploadFence);

  vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::init_descriptors() {
  // new code abstract

  _descriptorAllocator = new vkutil::DescriptorAllocator{};
  _descriptorAllocator->init(_device);

  _descriptorLayoutCache = new vkutil::DescriptorLayoutCache{};
  _descriptorLayoutCache->init(_device);

  /*Creating Layout bindings GLOBAL*/

  Material *texturedMat = create_material("texturedmesh");
  VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

  VkSampler blockySampler;
  vkCreateSampler(_device, &samplerInfo, nullptr, &blockySampler);

  _blockySampler = blockySampler;

  VkDescriptorImageInfo imageBufferInfo;
  imageBufferInfo.sampler = blockySampler;
  imageBufferInfo.imageView = _loadedTextures["empire_diffuse"].imageView;
  imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  vkutil::DescriptorBuilder::begin(_descriptorLayoutCache, _descriptorAllocator)
      .bind_image(0, &imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
      .build(texturedMat->textureSet, _singleTextureSetLayout);

  /*Creation of Binding Resource*/
  const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));

  _sceneParameterBuffer =
      create_buffer(sceneParamBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  for (int i = 0; i < FRAME_OVERLAP; i++) {
    /*Initilization of allocators*/
    _frames[i].dynamicDescriptorAllocator = new vkutil::DescriptorAllocator{};
    _frames[i].dynamicDescriptorAllocator->init(_device);

    _frames[i].layoutCacheAllocator = new vkutil::DescriptorLayoutCache();
    _frames[i].layoutCacheAllocator->init(_device);

    _frames[i].cameraBuffer =
        create_buffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    const int MAX_OBJECTS = 10000;
    _frames[i].objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            VMA_MEMORY_USAGE_CPU_TO_GPU);

    _openglFrames[i].objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS,
                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    /*Creating Layout bindings OBJECT*/
    VkDescriptorBufferInfo cameraInfo;
    cameraInfo.buffer = _frames[i].cameraBuffer._buffer;
    cameraInfo.offset = 0;
    cameraInfo.range = sizeof(GPUCameraData);

    VkDescriptorBufferInfo sceneInfo;
    sceneInfo.buffer = _sceneParameterBuffer._buffer;
    sceneInfo.offset = 0;
    sceneInfo.range = sizeof(GPUSceneData);

    VkDescriptorBufferInfo objectBufferInfo;
    objectBufferInfo.buffer = _frames[i].objectBuffer._buffer;
    objectBufferInfo.offset = 0;
    objectBufferInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

    VkDescriptorBufferInfo objectOpenglInfo;
    objectOpenglInfo.buffer = _openglFrames[i].objectBuffer._buffer;
    objectOpenglInfo.offset = 0;
    objectOpenglInfo.range = sizeof(GPUObjectData) * MAX_OBJECTS;

    vkutil::DescriptorBuilder::begin(_frames[i].layoutCacheAllocator, _frames[i].dynamicDescriptorAllocator)
        .bind_buffer(0, &cameraInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)

        .bind_buffer(1, &sceneInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT)

        .build(_frames[i].globalDescriptor, _globalSetLayout);

    vkutil::DescriptorBuilder::begin(_frames[i].layoutCacheAllocator, _frames[i].dynamicDescriptorAllocator)
        .bind_buffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build(_frames[i].objectDescriptor, _objectSetLayout);

    /*Opengl Descriptors*/

    vkutil::DescriptorBuilder::begin(_frames[i].layoutCacheAllocator, _frames[i].dynamicDescriptorAllocator)
        .bind_buffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build(_openglFrames[i].objectDescriptor, _objectOpenglLayout);
  }

  _mainDeletionQueue.push_function([&]() {
    vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

    auto pointer = _objectSetLayout;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
      vmaDestroyBuffer(_allocator, _frames[i].cameraBuffer._buffer, _frames[i].cameraBuffer._allocation);
      vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

      _frames[i].layoutCacheAllocator->cleanup();
      _frames[i].dynamicDescriptorAllocator->cleanup();
    }
    _descriptorAllocator->cleanup();
    _descriptorLayoutCache->cleanup();
  });
}
