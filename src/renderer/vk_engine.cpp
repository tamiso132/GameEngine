#include "vk_engine.h"

#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <cstdio>
#include <filesystem>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "util/vk_initializers.h"
#include "vk_create.h"
#include "vk_mesh.h"
#include "vk_types.h"

#define VMA_IMPLEMENTATION
#include "imgui_impl_sdl2.h"
#include "util/helper.h"
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

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.

    std::unordered_map<std::string, VkShaderModule> shaderModules;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow("Vulkan Engine", 0, 0, _windowExtent.width, _windowExtent.height, window_flags);

    init_vulkan();

    init_swapchain();

    init_commands();

    init_sync_structures();

    Helper::init(this->_device, this->_gpuProperties, this->_allocator, this->cmd, this->_graphicsQueue);
    for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
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

    init_hdr();

    init_descriptors();

    init_pipelines(shaderModules);

    // TODO, a check if more then 1 display
    SDL_Rect rect;
    SDL_GetWindowSize(_window, &rect.w, &rect.h);
    printf("WINDOW WIDTH: %d\n", rect.w);
    printf("WINDOW HEIGHT: %d\n", rect.h);
    // SDL_SetWindowPosition(_window, rect.x, rect.y);
    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::cleanup() {}

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

    VkRenderingAttachmentInfo colorAttachment = {};

    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.clearValue = clearValue;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.imageView = _swapchainImageViews[swapchainImageIndex];
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = {};

    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.clearValue = depthClear;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.imageView = _depthImageView;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfoKHR dynamicInfo = {};
    dynamicInfo.pNext = nullptr;
    dynamicInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    dynamicInfo.colorAttachmentCount = 1;
    dynamicInfo.pColorAttachments = &colorAttachment;
    dynamicInfo.pDepthAttachment = &depthAttachment;
    dynamicInfo.renderArea = {{0, 0}, _windowExtent};
    dynamicInfo.layerCount = 1;

    Helper::transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, _swapchainImages[swapchainImageIndex], cmd);
    deviceFunctions.cmdBeginRenderingKHR(cmd, &dynamicInfo);

    draw_test();

    deviceFunctions.cmdEndRenderingKHR(cmd);

    Helper::transition_image_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, _swapchainImages[swapchainImageIndex], cmd);

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
    this->deviceFunctions = vkbDevice.make_table();
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
                                      .set_desired_format({VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                      // use vsync present mode
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(_windowExtent.width, _windowExtent.height)
                                      .build()
                                      .value();
    printf("SWAPCHAIN HEIGHT: %d\n", vkbSwapchain.extent.height);
    printf("SWAPCHAIN WIDTH: %d\n", vkbSwapchain.extent.width);

    // store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

    _swapchainImageFormat = vkbSwapchain.image_format;

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

    VkDescriptorSetLayout layoutHdr[] = {this->global.get_descriptor_layout("hdr")};
    triangleLayoutInfo.pSetLayouts = layoutHdr;
    triangleLayoutInfo.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(this->_device, &triangleLayoutInfo, nullptr, &this->hdrLayout));
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

    VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.pColorAttachmentFormats = &this->_swapchainImageFormat;
    pipelineCreateInfo.colorAttachmentCount = 1;
    pipelineCreateInfo.depthAttachmentFormat = _depthFormat;

    this->pipeline = pipelineBuilder.build_pipeline(_device, pipelineCreateInfo);

    pipelineBuilder._pipelineLayout = hdrLayout;

    pipelineBuilder._shaderStages.clear();

    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, shaders["hdr.frag.spv"]));

    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = nullptr;
    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = 0;
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = 0;

    this->hdrPipeline = pipelineBuilder.build_pipeline(_device, pipelineCreateInfo);
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkPipelineRenderingCreateInfoKHR pass) {
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

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.flags = 0;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable = VK_FALSE; // You can set this to VK_TRUE if you want to enable sample shading
    multisampleInfo.pNext = nullptr;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pass;
    pipelineInfo.renderPass = nullptr;

    pipelineInfo.pMultisampleState = &multisampleInfo;

    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &_depthStencil;
    pipelineInfo.layout = _pipelineLayout;
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

void VulkanEngine::init_hdr() { Helper::create_image({_windowExtent.width, _windowExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &hdrImage, &hdrImageView); }
void VulkanEngine::init_descriptors() {
    // // new code abstract

    VkSamplerCreateInfo sampler = vkinit::sampler_create_info(VK_FILTER_NEAREST);

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

    VkSamplerCreateInfo hdrSamplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

    vkCreateSampler(_device, &sampler, nullptr, &blockySampler);
    vkCreateSampler(_device, &hdrSamplerInfo, nullptr, &this->hdrSampler);

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
    builder.bind_create_buffer(sizeof(GPUObject) * MAX_OBJECTS, BufferType::STORAGE, VK_SHADER_STAGE_VERTEX_BIT).update_descriptor(true).build("object");

    GlobalBuilder builder2 = this->global.begin_build_descriptor();
    builder2.bind_create_buffer(sizeof(GPUCamera), BufferType::UNIFORM, VK_SHADER_STAGE_VERTEX_BIT).update_descriptor(true).build("camera");

    GlobalBuilder builder3 = this->global.begin_build_descriptor();
    builder3
        .bind_image(&imageBufferInfo, ImageType::COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // yep
        ->bind_create_buffer(sizeof(GPUTexture), BufferType::UNIFORM, VK_SHADER_STAGE_FRAGMENT_BIT)
        .bind_image(&normalImageBufferInfo, ImageType::COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        ->update_descriptor(true)
        .build("cubemap");

    VkDescriptorImageInfo imageInfo;
    imageInfo.imageView = hdrImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler = _blockSampler;

    GlobalBuilder builder4 = this->global.begin_build_descriptor();
    builder4.bind_image(&imageInfo, ImageType::COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)->build("hdr");

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
