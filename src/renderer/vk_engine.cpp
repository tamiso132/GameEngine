#include "vk_engine.h"

#include <SDL_stdinc.h>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "VkBootstrap.h"
#include "util/vk_initializers.h"
#include "vk_create.h"
#include "vk_mesh.h"
#include "vk_types.h"

#define VMA_IMPLEMENTATION
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "util/helper.h"
#include "util/vk_descriptors.h"
#include "vk_mem_alloc.h"

#include "object/mesh.h"

constexpr bool bUseValidationLayers = true;

const char *directoryPath = "shaders/spiv/";
namespace fs = std::filesystem;

// we want to immediately abort when there is an error. In normal engines this
// would give an error message to the user, or perform a dump of state.
using namespace std;

std::vector<const char *> device_extensions = {"VK_KHR_dynamic_rendering"};

const uint32_t MAX_OBJECTS = 20;

struct Light {
    glm::vec4 position;
    glm::vec3 color;
    float radius;
};

struct {
    AllocatedBuffer GBuffer;
    AllocatedBuffer lights;
} buffers;

struct FrameBufferAttachment {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format;
};

struct Attachments {
    FrameBufferAttachment position, normal, albedo;
    int32_t width;
    int32_t height;
} attachments;

std::array<Light, 64> lights;

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.

    std::unordered_map<std::string, VkShaderModule> shaderModules;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowExtent.width, _windowExtent.height, window_flags);

    init_vulkan();

    init_swapchain();

    init_default_renderpass();

    init_framebuffers();

    init_commands();

    init_sync_structures();

    Helper::init(this->_device, this->_gpuProperties, this->_allocator, this->cmd, this->_graphicsQueue);
    for (const auto &entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry.path())) {
            VkShaderModule shader;
            Helper::load_shader_module(entry.path().c_str(), &shader);
            auto c = entry.path().c_str();
            auto filename = Helper::get_filename_from_path(c);
            shaderModules[filename] = shader;
        }
    }

    init_mesh();
    Block::init_texture();

    init_descriptors();

    init_pipelines(shaderModules);

    create_vertex_buffer();

    // TODO, a check if more then 1 display
    // SDL_Rect rect;
    // SDL_GetDisplayBounds(1, &rect);
    // SDL_SetWindowPosition(_window, rect.x, rect.y);
    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::cleanup() {}

const std::vector<uint16_t> indices = {
    0, 1, 2, // front Face cube
    2, 3, 1, // front Face cube

    1, 5, 3, // Right Face Cube
    3, 7, 5, // Right Face Cube

    0, 4, 2, // Left Face Cube
    2, 6, 4, // Left Face Cube

    4, 5, 6, // Back Face Cube,
    6, 7, 5, // Back Face Cube

    4, 5, 0, // Top Face Cube
    0, 1, 4, // Top Face Cube

    6, 7, 2, // Bottom Face Cube
    2, 3, 7, // Bottom Face Cube
};

void VulkanEngine::create_vertex_buffer() {}

void VulkanEngine::draw_test() {
    /*Camera*/
    auto view = _cam.get_view();
    //  camera projection
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    GPUCamera camData;
    camData.camPos = _cam.get_camera_position();
    camData.viewproj = projection * view;

    void *data;
    this->global.write_descriptor_set("camera", 0, this->_allocator, &camData, sizeof(GPUCamera));

    glm::vec3 position = glm::vec3(0, 0, 0);
    std::vector<GPUObject> objects;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        position.x += 1;
        glm::mat4 positionMatrix = glm::translate(glm::mat4(1.0f), position);
        GPUObject object;
        object.transformMatrix = positionMatrix;

        objects.push_back(object);
    }

    this->global.write_descriptor_set("object", 0, this->_allocator, objects.data(), sizeof(GPUObject) * MAX_OBJECTS);

    DescriptorSet cameraSet = global.get_descriptor_set("camera");
    DescriptorSet objectSet = global.get_descriptor_set("object");
    DescriptorSet cubemap = global.get_descriptor_set("cubemap");

    vkCmdBindPipeline(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

    vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 0, 1, &cameraSet.descriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 1, 1, &objectSet.descriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 2, 1, &cubemap.descriptorSet, 0, nullptr);
    VkDeviceSize offset = 0;
    auto verticesBuffer = Cube::get_vertices_buffer()._buffer;
    vkCmdBindVertexBuffers(this->cmd, 0, 1, &verticesBuffer, &offset);

    for (int i = 0; i < MAX_OBJECTS; i++) {
        vkCmdDraw(this->cmd, Cube::get_vertices_size(), 1, 0, i);
    }
}

void VulkanEngine::draw() {
    // // check if window is minimized and skip drawing
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED)
        return;

    /*SYNC FRAME*/
    VK_CHECK(vkWaitForFences(_device, 1, &this->fence_wait, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &this->fence_wait));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, this->present_semp, nullptr, &swapchainImageIndex));

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkResetCommandBuffer(this->cmd, 0));
    VK_CHECK(vkBeginCommandBuffer(this->cmd, &cmdBeginInfo));

    /*Render Pass*/
    VkClearValue clearValue;
    clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f};
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);

    // connect clear values
    rpInfo.clearValueCount = 2;

    VkClearValue clearValues[] = {clearValue, depthClear};
    rpInfo.pClearValues = &clearValues[0];

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    draw_test();

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    /*Submit Queue*/
    VkSubmitInfo submit = vkinit::submit_info(&cmd);
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    submit.pWaitDstStageMask = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &this->present_semp;

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &this->render_semp;

    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, this->fence_wait));

    VkPresentInfoKHR presentInfo = vkinit::present_info();

    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &this->render_semp;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber++;
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    const double target_fps = 60.0;
    const double target_frame_duration = 1.0 / target_fps;

    float delta_time = 0;
    auto last_frame = 0.0f;

    SDL_bool mouse_inside_window = SDL_FALSE;
    SDL_SetRelativeMouseMode(mouse_inside_window); // fix true later

    int relX, relY;
    bool move_keys[] = {0, 0, 0, 0};
    // main loop
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);
    while (!bQuit) {
        float current_frame = (float)SDL_GetTicks64() / 1000;

        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        SDL_GetRelativeMouseState(&relX, &relY);
        _cam.process_input(nullptr, delta_time, relX, relY, keystate, mouse_inside_window);
        while (SDL_PollEvent(&e) != 0) {
            // ImGui_ImplSDL2_ProcessEvent(&e);
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
        draw();
    }
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application").request_validation_layers(bUseValidationLayers).use_default_debug_messenger().require_api_version(1, 3, 0).build();

    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    VkPhysicalDeviceFeatures features;
    features.imageCubeArray = true;
    features.samplerAnisotropy = true;

    VkPhysicalDeviceVulkan11Features features_11;
    features_11.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan13Features features_13;
    features_13.dynamicRendering = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                             .set_required_features(features) // features for 1.0
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

    vkGetPhysicalDeviceProperties(_chosenGPU, &_gpuProperties);

    std::cout << "The gpu has a minimum buffer alignement of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
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

    // depth image size will match the window
    VkExtent3D depthImageExtent = {_windowExtent.width, _windowExtent.height, 1};

    // hardcoding the depth format to 32 bit float
    _depthFormat = VK_FORMAT_D32_SFLOAT;

    // the depth image will be a image with the format we selected and Depth
    // Attachment usage flag
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    // for the depth image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

    // build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));
}

void VulkanEngine::init_default_renderpass() {

    attachments.width = _windowExtent.width;
    attachments.height = _windowExtent.height;

    // createGBufferAttachments();

    std::array<VkAttachmentDescription, 5> attachmentsDesc{};
    // Color attachment
    attachmentsDesc[0].format = _swachainImageFormat;
    attachmentsDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentsDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Deferred attachments
    // Position
    attachmentsDesc[1].format = attachments.position.format;
    attachmentsDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Normals
    attachmentsDesc[2].format = attachments.normal.format;
    attachmentsDesc[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Albedo
    attachmentsDesc[3].format = attachments.albedo.format;
    attachmentsDesc[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth attachment
    attachmentsDesc[4].format = _depthFormat;
    attachmentsDesc[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Three subpasses
    std::array<VkSubpassDescription, 3> subpassDescriptions{};

    // First subpass: Fill G-Buffer components
    // ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReferences[4];
    colorReferences[0] = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    colorReferences[1] = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    colorReferences[2] = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    colorReferences[3] = {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthReference = {4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].colorAttachmentCount = 4;
    subpassDescriptions[0].pColorAttachments = colorReferences;
    subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

    // Second subpass: Final composition (using G-Buffer components)
    // ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentReference inputReferences[3];
    inputReferences[0] = {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    inputReferences[1] = {2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    inputReferences[2] = {3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &colorReference;
    subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
    // Use the color attachments filled in the first pass as input attachments
    subpassDescriptions[1].inputAttachmentCount = 3;
    subpassDescriptions[1].pInputAttachments = inputReferences;

    // Third subpass: Forward transparency
    // ----------------------------------------------------------------------------------------
    colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    inputReferences[0] = {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[2].colorAttachmentCount = 1;
    subpassDescriptions[2].pColorAttachments = &colorReference;
    subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
    // Use the color/depth attachments filled in the first pass as input attachments
    subpassDescriptions[2].inputAttachmentCount = 1;
    subpassDescriptions[2].pInputAttachments = inputReferences;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 5> dependencies;

    // This makes sure that writes to the depth image are done before we try to write to it again
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = 0;

    // This dependency transitions the input attachment from color attachment to input attachment read
    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = 1;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[3].srcSubpass = 1;
    dependencies[3].dstSubpass = 2;
    dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[4].srcSubpass = 2;
    dependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[4].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentsDesc.size());
    renderPassInfo.pAttachments = attachmentsDesc.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassInfo.pSubpasses = subpassDescriptions.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass));
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
    }
}

void VulkanEngine::init_commands() {
    // // create a command pool for commands submitted to the graphics queue.
    // // we also want the pool to allow for resetting of individual command
    // buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(this->_device, &commandPoolInfo, nullptr, &this->pool));
    VkCommandBufferAllocateInfo bufferInfo = vkinit::command_buffer_allocate_info(this->pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(this->_device, &bufferInfo, &this->cmd));
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo sempahoreInfo = vkinit::semaphore_create_info();

    VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &this->fence_wait));

    VK_CHECK(vkCreateSemaphore(this->_device, &sempahoreInfo, nullptr, &this->present_semp));
    VK_CHECK(vkCreateSemaphore(this->_device, &sempahoreInfo, nullptr, &this->render_semp));
}

void VulkanEngine::init_pipelines(std::unordered_map<std::string, VkShaderModule> &shaders) {

    PipelineBuilder pipelineBuilder;
    /*SHADERS PUSH*/
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, shaders["colored_triangle.vert.spv"]));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, shaders["colored_triangle.frag.spv"]));

    /*Layout PUSH*/
    VkPipelineLayoutCreateInfo triangleLayoutInfo = vkinit::pipeline_layout_create_info();

    VkDescriptorSetLayout layouts[] = {this->global.get_descriptor_layout("camera"), this->global.get_descriptor_layout("object"), this->global.get_descriptor_layout("cubemap")};

    triangleLayoutInfo.pSetLayouts = layouts;
    triangleLayoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);

    VK_CHECK(vkCreatePipelineLayout(this->_device, &triangleLayoutInfo, nullptr, &this->pipelineLayout));

    /*Pipeline Settings*/
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._pipelineLayout = this->pipelineLayout;

    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // /*VIEWPORT*/
    // // build viewport and scissor from the swapchain extents
    pipelineBuilder._viewport.x = 0.0f;
    pipelineBuilder._viewport.y = 0.0f;
    pipelineBuilder._viewport.width = (float)_windowExtent.width;
    pipelineBuilder._viewport.height = (float)_windowExtent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;

    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = _windowExtent;

    // /*Extra*/VkCullModeFlagBits
    // // configure the rasterizer to draw filled triangles
    pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    // // we dont use multisampling, so just run the default one
    pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

    // // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // // default depthtesting
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // /*Vertex Bindings*/

    auto description = vertex_input_description();

    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = description.attributes.data();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = description.bindings.data();

    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = description.attributes.size();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = description.bindings.size();

    this->pipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
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
    // we now use all of the info structs we have been writing into into this
    // one to create the pipeline
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
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
        return newPipeline;
    }
}

void VulkanEngine::init_descriptors() {
    // // new code abstract

    VkSamplerCreateInfo sampler = vkinit::sampler_create_info(VK_FILTER_LINEAR);

    // TODO, fix minimapping
    VkSampler blockySampler;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.mipLodBias = 0.0f;
    sampler.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.anisotropyEnable = VK_FALSE;

    vkCreateSampler(_device, &sampler, nullptr, &blockySampler);
    _blockySampler = blockySampler;

    TextureHelper::load_texture_array("assets/texture_atlas_0.png", 64, _cubemap, &_cubeview);

    TextureHelper::load_texture_array("assets/texture_atlas_0.png", 64, _normalMap, &_normalView);

    VkDescriptorImageInfo imageBufferInfo;
    imageBufferInfo.sampler = blockySampler;
    imageBufferInfo.imageView = _cubeview;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo normalImageBufferInfo;
    normalImageBufferInfo.sampler = blockySampler;
    normalImageBufferInfo.imageView = _normalView;
    normalImageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    this->global.init(this->_device);

    GlobalBuilder builder = this->global.begin_build_descriptor();
    builder.bind_create_buffer(sizeof(GPUObject) * MAX_OBJECTS, BufferType::STORAGE, VK_SHADER_STAGE_VERTEX_BIT).build("object");

    GlobalBuilder builder2 = this->global.begin_build_descriptor();
    builder2.bind_create_buffer(sizeof(GPUCamera), BufferType::UNIFORM, VK_SHADER_STAGE_VERTEX_BIT).build("camera");

    GlobalBuilder builder3 = this->global.begin_build_descriptor();
    builder3
        .bind_image(&imageBufferInfo, ImageType::COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // yep
        ->bind_create_buffer(sizeof(GPUTexture), BufferType::UNIFORM, VK_SHADER_STAGE_FRAGMENT_BIT)
        .bind_image(&normalImageBufferInfo, ImageType::COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        ->build("cubemap");

    GPUTexture textureIndices = Block::get_texture(Block::Type::ACACIA_TREE);

    this->global.write_descriptor_set("cubemap", 1, _allocator, &textureIndices, sizeof(GPUTexture));
}

void VulkanEngine::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkResetCommandBuffer(this->cmd, 0));
    VK_CHECK(vkBeginCommandBuffer(this->cmd, &cmdBeginInfo));

    // Record the copy command
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    // End the command buffer recording

    // Submit the command buffer for execution (Assuming you have a Vulkan queue)
    VkSubmitInfo submit = vkinit::submit_info(&cmd);

    VK_CHECK(vkQueueSubmit(this->_graphicsQueue, 1, &submit, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(this->_graphicsQueue)); // Optional: Wait for the transfer to complete
}
