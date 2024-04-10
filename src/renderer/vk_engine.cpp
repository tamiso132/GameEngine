#include "vk_engine.h"

#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <cstdio>
#include <filesystem>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "VkBootstrap.h"
#include "VkBootstrapDispatch.h"
#include "mesh.h"
#include "pipeline.h"
#include "util/vk_initializers.h"
#include "vk_resource.h"
#include "vk_types.h"

#define VMA_IMPLEMENTATION
#include "imgui_impl_sdl2.h"
#include "util/helper.h"
#include "vk_mem_alloc.h"

constexpr bool bUseValidationLayers = true;

// we want to immediately abort when there is an error. In normal engines this
// would give an error message to the user, or perform a dump of state.
using namespace std;

std::vector<const char *> device_extensions = {"VK_KHR_dynamic_rendering"};

const uint32_t MAX_OBJECTS = 20;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

namespace util {
    VkCommandBuffer create_cmd(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPool pool) {
        VkCommandBuffer cmd;

        VkCommandBufferAllocateInfo bufferInfo = vkinit::command_buffer_allocate_info(pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(context.device, &bufferInfo, &cmd));

        return cmd;
    };

    VkCommandPool create_pool(VulkanContextGlobal context, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag) {
        VkCommandPool pool;

        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(queueFamilyIndex, flag);
        VK_CHECK(vkCreateCommandPool(context.device, &commandPoolInfo, nullptr, &pool));

        return pool;
    }
    VkFence create_fence(VulkanContextGlobal context) {
        VkFence fence;

        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        vkCreateFence(context.device, &fenceCreateInfo, nullptr, &fence);

        return fence;
    }
    VkSemaphore create_semaphore(VulkanContextGlobal context) {
        VkSemaphore semaphore;

        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
        vkCreateSemaphore(context.device, &semaphoreCreateInfo, nullptr, &semaphore);

        return semaphore;
    }

}; // namespace util

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    ci_vulkanGlobal.window = SDL_CreateWindow("Vulkan Engine", 0, 0, ci_vulkanGlobal.windowExtent.width, ci_vulkanGlobal.windowExtent.height, window_flags);
    vkb::DispatchTable table;

    init_vulkan(table);

    pool = util::create_pool(ci_vulkanGlobal, _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    cmd  = util::create_cmd(ci_vulkanGlobal, _graphicsQueueFamily, pool);

    ci_resourceContext.init(ci_vulkanGlobal.chosenGPU, ci_vulkanGlobal.device, ci_vulkanGlobal.instance, {_transferQueue, _transferQueueFamily}, {_graphicsQueue, _graphicsQueueFamily}, cmd,
                            table.fp_vkSetDebugUtilsObjectNameEXT);

    ci_meshes.init_mesh(ci_resourceContext);

    auto pngContext   = load_png("assets/texture_atlas_0.png");
    auto pngNormalMap = load_png("assets/texture_atlas_0_normalmap.png");

    ResourceBuilder dynamicBuilder{};
    ResourceBuilder staticBuilder{};

    dynamicBuilder.add_buffer(sizeof(GPUObject) * MAX_OBJECTS, BufferType::Storage, "objects", MemoryType::HostVisible);
    dynamicBuilder.add_buffer(sizeof(GPUCamera), BufferType::Uniform, "camera", MemoryType::HostVisible);

    staticBuilder.add_image_array_with_view(VK_FORMAT_R8G8B8A8_SRGB, pngContext.pixelSize, 64, pngContext.texWidth, pngContext.texHeight, "texture_atlas", pngContext.pixels);
    staticBuilder.add_image_array_with_view(VK_FORMAT_R8G8B8A8_SRGB, pngNormalMap.pixelSize, 64, pngNormalMap.texWidth, pngNormalMap.texHeight, "texture_atlas_normal", pngNormalMap.pixels);
    staticBuilder.add_buffer(sizeof(GPUTexture) * 5, BufferType::Storage, "materials");

    ResourceIndex textureAtlasIndex = ci_resourceContext.build_resource_blueprint(staticBuilder.get_blueprint());

    std::vector<FrameData> frames;

    VkDescriptorSetLayout layout = nullptr;

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
    ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
    ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});

    ci_globalDescriptorAllocator.init(ci_vulkanGlobal.device, 5, ratios);

    DescriptorLayoutBuilder textureLayoutBuilder;
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas"));
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas_normal"));
    textureLayoutBuilder.add_buffer(ci_resourceContext.get_resource_by_name("materials"), VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout textureLayout = ci_resourceContext.build_descriptor_layout_blueprint(textureLayoutBuilder.get_blueprint(), ci_globalDescriptorAllocator, VK_SHADER_STAGE_FRAGMENT_BIT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameData frame;

        frame.imageAquired = util::create_semaphore(ci_vulkanGlobal);
        frame.presentReady = util::create_semaphore(ci_vulkanGlobal);
        frame.queueDone    = util::create_fence(ci_vulkanGlobal);

        frame.pool = util::create_pool(ci_vulkanGlobal, _graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        frame.cmd  = util::create_cmd(ci_vulkanGlobal, _graphicsQueueFamily, frame.pool);

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
        ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});
        frame.descriptorAllocator.init(ci_vulkanGlobal.device, 5, ratios);

        char c;
        if (i == 0) {
            c = '\0';
        } else {
            c = static_cast<char>(i - 1);
        }

        ci_resourceContext.build_resource_blueprint(dynamicBuilder.get_blueprint());

        auto cameraIndex  = ci_resourceContext.get_resource_by_name("camera" + c);
        auto objectsIndex = ci_resourceContext.get_resource_by_name("objects" + c);

        DescriptorLayoutBuilder layoutBuilder;

        layoutBuilder.add_buffer(cameraIndex, VK_SHADER_STAGE_VERTEX_BIT);
        layoutBuilder.add_buffer(objectsIndex, VK_SHADER_STAGE_VERTEX_BIT);

        layout = ci_resourceContext.build_descriptor_layout_blueprint(layoutBuilder.get_blueprint(), frame.descriptorAllocator, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, layout);

        frames.push_back(frame);
    };

    /*Pipeline Stuff*/

    pipeline.init_pipeline(ci_vulkanGlobal.device);

    PipelineBuilder pipelineBuilder = PipelineBuilder::default_pipeline_builder(ci_vulkanGlobal, "colored_triangle.vert.spv", "colored_triangle.frag.spv");

    pipelineBuilder.add_attachments_formats({_swapchainImageFormat}, _depthFormat);
    pipelineBuilder.add_depth(true, false, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder.add_cull_mode(VK_CULL_MODE_BACK_BIT);
    pipelineBuilder.add_vertex_attribute(VertexBlock::vertex_input_description());
    pipelineBuilder.add_layout_sets({textureLayout, layout});

    pipeline.build_pipeline(pipelineBuilder, "main_pipeline");
    _isInitialized = true;
}

void VulkanEngine::cleanup() {}

void VulkanEngine::draw_test() {
    // /*Camera*/
    // auto view = _cam.get_view();
    // //  camera projection

    // glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    // projection[1][1] *= -1;

    // GPUCamera camData;
    // camData.camPos   = _cam.get_camera_position();
    // camData.viewproj = projection * view;

    // void *data;
    // this->global.write_descriptor_set("camera", 0, this->_allocator, &camData, sizeof(GPUCamera));

    // glm::vec3              position = glm::vec3(0, 0, 0);
    // std::vector<GPUObject> objects;
    // for (int i = 0; i < MAX_OBJECTS; i++) {
    //     position.x += 1;
    //     glm::mat4 positionMatrix = glm::translate(glm::mat4(1.0f), position);
    //     GPUObject object;
    //     object.transformMatrix = positionMatrix;

    //     objects.push_back(object);
    // }
    // //     std::memcpy(dstData, srcData, srcSize);
    // //     vmaUnmapMemory(allocator, set.bindingsPointers[bindingIndex].value()._allocation);
    // DescriptorWriter writer;
    // // writer.write_buffer(0, objects, size_t size, size_t offset, VkDescriptorType type)
    // this->global.write_descriptor_set("object", 0, this->_allocator, objects.data(), sizeof(GPUObject) * MAX_OBJECTS);

    // DescriptorSet cameraSet = global.get_descriptor_set("camera");
    // DescriptorSet objectSet = global.get_descriptor_set("object");
    // DescriptorSet cubemap   = global.get_descriptor_set("cubemap");

    // vkCmdBindPipeline(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

    // vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 0, 1, &cameraSet.descriptorSet, 0, nullptr);

    // vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 1, 1, &objectSet.descriptorSet, 0, nullptr);
    // vkCmdBindDescriptorSets(this->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelineLayout, 2, 1, &cubemap.descriptorSet, 0, nullptr);
    // VkDeviceSize offset         = 0;
    // auto         verticesBuffer = Cube::get_vertices_buffer()._buffer;
    // vkCmdBindVertexBuffers(this->cmd, 0, 1, &verticesBuffer, &offset);

    // for (int i = 0; i < MAX_OBJECTS; i++) {
    //     vkCmdDraw(this->cmd, Cube::get_vertices_size(), 1, 0, i);
    // }
}

void VulkanEngine::draw() {
    // // // check if window is minimized and skip drawing
    // if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED)
    //     return;

    // /*SYNC FRAME*/
    // VK_CHECK(vkWaitForFences(ci_vulkanGlobal.device, 1, &this->fence_wait, true, 1000000000));
    // VK_CHECK(vkResetFences(ci_vulkanGlobal.device, 1, &this->fence_wait));

    // uint32_t swapchainImageIndex;
    // VK_CHECK(vkAcquireNextImageKHR(ci_vulkanGlobal.device, _swapchain, 1000000000, this->present_semp, nullptr, &swapchainImageIndex));

    // VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // VK_CHECK(vkResetCommandBuffer(this->cmd, 0));
    // VK_CHECK(vkBeginCommandBuffer(this->cmd, &cmdBeginInfo));

    // VkClearValue clearValue;
    // clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f};
    // VkClearValue depthClear;
    // depthClear.depthStencil.depth = 1.f;

    // VkRenderingAttachmentInfo colorAttachment = {};

    // colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // colorAttachment.clearValue  = clearValue;
    // colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    // colorAttachment.imageView   = _swapchainImageViews[swapchainImageIndex];
    // colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    // colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

    // VkRenderingAttachmentInfo depthAttachment = {};

    // depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // depthAttachment.clearValue  = depthClear;
    // depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // depthAttachment.imageView   = _depthImageView;
    // depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

    // VkRenderingInfoKHR dynamicInfo   = {};
    // dynamicInfo.pNext                = nullptr;
    // dynamicInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    // dynamicInfo.colorAttachmentCount = 1;
    // dynamicInfo.pColorAttachments    = &colorAttachment;
    // dynamicInfo.pDepthAttachment     = &depthAttachment;
    // dynamicInfo.renderArea           = {{0, 0}, ci_vulkanGlobal.windowExtent};
    // dynamicInfo.layerCount           = 1;

    // Helper::transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _swapchainImages[swapchainImageIndex], cmd);

    // deviceFunctions.cmdBeginRenderingKHR(cmd, &dynamicInfo);

    // draw_test();

    // deviceFunctions.cmdEndRenderingKHR(cmd);

    // Helper::transition_image_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, _swapchainImages[swapchainImageIndex], cmd);

    // VK_CHECK(vkEndCommandBuffer(cmd));

    // /*Submit Queue*/
    // VkSubmitInfo         submit    = vkinit::submit_info(&cmd);
    // VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // submit.pWaitDstStageMask = &waitStage;

    // submit.waitSemaphoreCount = 1;
    // submit.pWaitSemaphores    = &this->present_semp;

    // submit.signalSemaphoreCount = 1;
    // submit.pSignalSemaphores    = &this->render_semp;

    // VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, this->fence_wait));

    // VkPresentInfoKHR presentInfo = vkinit::present_info();

    // presentInfo.pSwapchains    = &_swapchain;
    // presentInfo.swapchainCount = 1;

    // presentInfo.pWaitSemaphores    = &this->render_semp;
    // presentInfo.waitSemaphoreCount = 1;

    // presentInfo.pImageIndices = &swapchainImageIndex;

    // VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    // _frameNumber++;
}

void VulkanEngine::run() {
    SDL_Event e;
    bool      bQuit = false;

    float delta_time = 0;
    auto  last_frame = 0.0f;

    SDL_bool mouse_inside_window = SDL_FALSE;
    SDL_SetRelativeMouseMode(mouse_inside_window); // fix true later

    int  relX, relY;
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

void VulkanEngine::init_vulkan(vkb::DispatchTable &table) {
    vkb::InstanceBuilder builder;
    // make the vulkan instance, with basic debug features
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(bUseValidationLayers)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .enable_extension("VK_EXT_debug_utils")
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    // grab the instance
    ci_vulkanGlobal.instance        = vkb_inst.instance;
    ci_vulkanGlobal.debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(ci_vulkanGlobal.window, ci_vulkanGlobal.instance, &_surface);

    VkPhysicalDeviceFeatures features;

    features.imageCubeArray    = true;
    features.samplerAnisotropy = true;
    VkPhysicalDeviceVulkan11Features features_11;
    features_11.shaderDrawParameters = true;
    VkPhysicalDeviceVulkan13Features features_13;
    features_13.dynamicRendering = true;

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice         physicalDevice = selector.set_minimum_version(1, 3)
                                             .set_required_features(features)
                                             .set_required_features_11(features_11)
                                             .set_required_features_13(features_13)
                                             .add_required_extension(device_extensions[0])
                                             .set_surface(_surface)
                                             .select()
                                             .value();

    // create the final vulkan device

    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    vkb::Device        vkbDevice = deviceBuilder.build().value();

    table = vkbDevice.make_table();

    // Get the VkDevice handle used in the rest of a vulkan application
    ci_vulkanGlobal.device    = vkbDevice.device;
    ci_vulkanGlobal.chosenGPU = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();

    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    _transferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = ci_vulkanGlobal.chosenGPU;
    allocatorInfo.device                 = ci_vulkanGlobal.device;
    allocatorInfo.instance               = ci_vulkanGlobal.instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);

    vkGetPhysicalDeviceProperties(ci_vulkanGlobal.chosenGPU, &_gpuProperties);

    std::cout << "The gpu has a minimum buffer alignement of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;

    vkb::SwapchainBuilder swapchainBuilder{ci_vulkanGlobal.chosenGPU, ci_vulkanGlobal.device, _surface};

    vkb::Swapchain vkbSwapchain = swapchainBuilder
                                      .set_desired_format({VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(ci_vulkanGlobal.windowExtent.width, ci_vulkanGlobal.windowExtent.height)
                                      .build()
                                      .value();

    // store swapchain and its related images
    _swapchain             = vkbSwapchain.swapchain;
    ci_swapchainImages     = vkbSwapchain.get_images().value();
    ci_swapchainImageViews = vkbSwapchain.get_image_views().value();

    _swapchainImageFormat = vkbSwapchain.image_format;

    // depth image size will match the window
    VkExtent3D depthImageExtent = {ci_vulkanGlobal.windowExtent.width, ci_vulkanGlobal.windowExtent.height, 1};

    // hardcoding the depth format to 32 bit float
    _depthFormat = VK_FORMAT_D32_SFLOAT;

    // the depth image will be a image with the format we selected and Depth
    // Attachment usage flag
    VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    // for the depth image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

    // build a image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(ci_vulkanGlobal.device, &dview_info, nullptr, &_depthImageView));
}
