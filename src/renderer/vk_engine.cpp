#include "vk_engine.h"
#include "util/pipeline.h"
#include <cstdint>
#include <cstdio>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION

#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <glm/fwd.hpp>
#include <imgui.h>

#include "util/mesh.h"
#include "util/renderer.h"
#include "util/vk_initializers.h"
#include "util/vk_resource.h"
#include "util/vk_types.h"
#include "util/vkutil.h"

#include <vk_mem_alloc.h>

#include "imgui_impl_sdl2.h"

constexpr bool bUseValidationLayers = true;
// we want to immediately abort when there is an error. In normal engines this
// would give an error message to the user, or perform a dump of state.
using namespace std;

std::vector<const char *> device_extensions = {"VK_KHR_dynamic_rendering"};

const uint32_t MAX_OBJECTS = 16 * 16 * 256;

static uint32_t objectsToRender = 0;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

PFN_vkCmdBeginRenderingKHR cmdBeginRenderingKHR;
PFN_vkCmdEndRenderingKHR   cmdEndRenderingKHR;

// Initilizes all the basic things for renderer

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.
    vkb::DispatchTable table;

    Render::init(ci_vulkanGlobal, _allocator, table, ci_resourceContext, ci_meshes);

    ci_swapChainBaseIndex = ci_resourceContext.get_resource_by_name("swapchain0");

    cmdBeginRenderingKHR = table.fp_vkCmdBeginRenderingKHR;
    cmdEndRenderingKHR   = table.fp_vkCmdEndRenderingKHR;

    auto pngContext   = load_png("assets/texture_atlas_0.png");
    auto pngNormalMap = load_png("assets/texture_atlas_0_normalmap.png");

    ResourceBuilder dynamicBuilder{};
    ResourceBuilder staticBuilder{};
    ResourceBuilder computeImageBuilder{};

    computeImageBuilder.add_image_with_view(VK_FORMAT_R16G16B16A16_SFLOAT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            VkExtent3D{ci_vulkanGlobal.windowExtent.width, ci_vulkanGlobal.windowExtent.height, 1}, "computeImage", VK_IMAGE_ASPECT_COLOR_BIT);

    dynamicBuilder.add_buffer(sizeof(GPUObject) * MAX_OBJECTS, BufferType::Storage, "objects", MemoryType::HostVisible);
    dynamicBuilder.add_buffer(sizeof(GPUCamera), BufferType::Uniform, "camera", MemoryType::HostVisible);

    staticBuilder.add_image_array_with_view(VK_FORMAT_R8G8B8A8_SRGB, pngContext.pixelSize, 64, pngContext.texWidth, pngContext.texHeight, "texture_atlas", pngContext.pixels);
    staticBuilder.add_image_array_with_view(VK_FORMAT_R8G8B8A8_SRGB, pngNormalMap.pixelSize, 64, pngNormalMap.texWidth, pngNormalMap.texHeight, "texture_atlas_normal", pngNormalMap.pixels);

    staticBuilder.add_buffer(sizeof(GPUTexture) * 1, BufferType::Storage, "materials", MemoryType::GpuOnly);

    ResourceIndex textureAtlasIndex = ci_resourceContext.build_resource_blueprint(staticBuilder.get_blueprint());

    ci_dynamicLayout = nullptr;

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
    ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
    ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});

    ci_globalDescriptorAllocator.init(ci_vulkanGlobal.device, 5, ratios);

    DescriptorLayoutBuilder textureLayoutBuilder;
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas"));
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas_normal"));
    textureLayoutBuilder.add_buffer(ci_resourceContext.get_resource_by_name("materials"), VK_SHADER_STAGE_FRAGMENT_BIT);

    ci_textureLayout = ci_resourceContext.build_descriptor_layout_blueprint(textureLayoutBuilder.get_blueprint(), ci_globalDescriptorAllocator);

    for (uint64_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameData frame;

        frame.imageAquired   = vkutil::create_semaphore(ci_vulkanGlobal);
        frame.presentReady   = vkutil::create_semaphore(ci_vulkanGlobal);
        frame.queueDoneFence = vkutil::create_fence(ci_vulkanGlobal);

        frame.pool = vkutil::create_pool(ci_vulkanGlobal, ci_vulkanGlobal.graphicQueue.queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        frame.cmd  = vkutil::create_cmd(ci_vulkanGlobal, ci_vulkanGlobal.graphicQueue.queueIndex, frame.pool);

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
        ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});
        frame.descriptorAllocator.init(ci_vulkanGlobal.device, 5, ratios);

        frame.dynamicIndex = ci_resourceContext.build_resource_blueprint(dynamicBuilder.get_blueprint());

        auto cameraIndex  = ci_resourceContext.get_resource_by_name("camera" + i);
        auto objectsIndex = ci_resourceContext.get_resource_by_name("objects" + i);
        if (i == 0) {
            cameraIndex  = ci_resourceContext.get_resource_by_name("camera");
            objectsIndex = ci_resourceContext.get_resource_by_name("objects");
        }
        DescriptorLayoutBuilder layoutBuilder;

        layoutBuilder.add_buffer(cameraIndex, VK_SHADER_STAGE_VERTEX_BIT);
        layoutBuilder.add_buffer(objectsIndex, VK_SHADER_STAGE_VERTEX_BIT);

        ci_dynamicLayout = ci_resourceContext.build_descriptor_layout_blueprint(layoutBuilder.get_blueprint(), frame.descriptorAllocator, ci_dynamicLayout);

        ci_frames.push_back(frame);

        /*Compute*/
        frame.computeIndex = ci_resourceContext.build_resource_blueprint(computeImageBuilder.get_blueprint());

        DescriptorLayoutBuilder skyBoxLayoutBuilder;
        skyBoxLayoutBuilder.add_image(frame.computeIndex);

        ci_skyboxLayout = ci_resourceContext.build_descriptor_layout_blueprint(skyBoxLayoutBuilder.get_blueprint(), frame.descriptorAllocator, ci_skyboxLayout);

        /*Upload Data*/
        std::vector<GPUObject> objectsUpload;

        GPUObject object{};
        glm::vec3 position = glm::vec3();

        for (int x = 0; x < 16; x++) {
            position.x = x;

            for (int z = 0; z < 16; z++) {
                position.z = z;

                for (int y = 0; y < 1; y++) {
                    position.y = y;

                    object.transformMatrix = glm::translate(glm::mat4(1.0f), position);
                    objectsUpload.push_back(object);
                }
            }
        }
        objectsToRender = 16 * 16 * 1;
        for (int i = 0; i < objectsToRender; i++) {
            position.x += 1;
            object.transformMatrix = glm::translate(glm::mat4(1.0f), position);
            objectsUpload.push_back(object);
        }
        ci_resourceContext.copy_to_buffer_visible(frame.dynamicIndex, objectsUpload.size() * sizeof(GPUObject), objectsUpload.data());
    };

    /*Pipeline Stuff*/

    pipeline.init_pipeline(ci_vulkanGlobal.device);

    PipelineBuilder pipelineBuilder = PipelineBuilder::default_builder(ci_vulkanGlobal, "colored_triangle.vert.spv", "colored_triangle.frag.spv");

    pipelineBuilder.add_attachments_formats({ci_vulkanGlobal.swapchainImageFormat}, ci_vulkanGlobal.depthImage.format);
    pipelineBuilder.add_depth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder.add_cull_mode(VK_CULL_MODE_BACK_BIT);
    pipelineBuilder.add_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.add_vertex_attribute(VertexBlock::vertex_input_description());
    pipelineBuilder.add_layout_sets({ci_textureLayout, ci_dynamicLayout});

    pipeline.build_pipeline(pipelineBuilder, "main_pipeline");
    ci_mainPipeline = pipeline.get_pipeline_layout("main_pipeline");

    ComputePipelineBuilder computeBuilder = ComputePipelineBuilder::default_builder("skybox.comp.spv");
    computeBuilder.add_layout_sets({ci_skyboxLayout});
    computeBuilder.add_push_constant(sizeof(SkyPushConstant));

    ci_skyboxPipeline_compute = pipeline.build_compute_pipeline(computeBuilder, "skybox_pipeline");
    _isInitialized            = true;
}

void VulkanEngine::cleanup() {}

void VulkanEngine::draw() {
    ZoneScoped;
    // // // check if window is minimized and skip drawing

    uint32_t frameIndex = ci_currentFrame % ci_frames.size();

    FrameData frame = ci_frames[frameIndex];

    if (SDL_GetWindowFlags(ci_vulkanGlobal.window) & SDL_WINDOW_MINIMIZED)
        return;

    uint32_t imageIndex;

    Render::prepare_frame(ci_vulkanGlobal, frame.cmd, frame.queueDoneFence, frame.imageAquired, imageIndex);

    /*Compute*/
    LayoutContext skyboxSet = ci_resourceContext.get_descriptor_sets_by_layout(ci_skyboxLayout);

    vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ci_skyboxPipeline_compute.pipeline);
    vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ci_skyboxPipeline_compute.layout, 0, 1, &skyboxSet.sets[frameIndex], 0, nullptr);

    SkyPushConstant sky{};
    sky.data1 = glm::vec4(1, 0, 0, 1);
    sky.data2 = glm::vec4(0, 0, 1, 1);

    vkCmdPushConstants(frame.cmd, ci_skyboxPipeline_compute.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyPushConstant), &sky);
    // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(frame.cmd, std::ceil(ci_vulkanGlobal.windowExtent.width / 16.0), std::ceil(ci_vulkanGlobal.windowExtent.height / 16.0), 1);

    VkClearValue clearValue;
    clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f};
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkRenderingAttachmentInfo colorAttachment =
        vkinit::render_attachment_info(clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR, ci_vulkanGlobal.swapchainImageViews[imageIndex], VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    VkRenderingAttachmentInfo depthAttachment =
        vkinit::render_attachment_info(depthClear, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ci_vulkanGlobal.depthImage.view, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    VkRenderingInfoKHR dynamicInfo = vkinit::render_info_khr(colorAttachment, &depthAttachment, ci_vulkanGlobal.windowExtent);

    vkutil::transition_image_color_attachment(frame.cmd, ci_vulkanGlobal.swapchainImages[imageIndex]);

    cmdBeginRenderingKHR(frame.cmd, &dynamicInfo);

    GPUCamera cameraUpload;

    glm::mat4x4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;

    cameraUpload.camPos   = _cam.get_camera_position();
    cameraUpload.viewproj = projection * _cam.get_view();

    ci_resourceContext.copy_to_buffer_visible(ci_resourceContext.cal_index(frame.dynamicIndex, 1), sizeof(GPUCamera), &cameraUpload);

    MeshContext mesh = ci_meshes.get_mesh(MeshType::CubeBlock);

    vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ci_mainPipeline.pipeline);

    LayoutContext dynamicLayout = ci_resourceContext.get_descriptor_sets_by_layout(ci_dynamicLayout);
    LayoutContext textureLayout = ci_resourceContext.get_descriptor_sets_by_layout(ci_textureLayout);

    vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ci_mainPipeline.layout, 0, 1, &textureLayout.sets[0], 0, nullptr);
    vkCmdBindDescriptorSets(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ci_mainPipeline.layout, 1, 1, &dynamicLayout.sets[frameIndex], 0, nullptr);

    VkDeviceSize offset{};
    vkCmdBindVertexBuffers(frame.cmd, 0, 1, &mesh.verticeBuffer._buffer, &offset);

    vkCmdDraw(frame.cmd, mesh.verticeSize, objectsToRender, 0, 0);
    cmdEndRenderingKHR(frame.cmd);

    vkutil::transition_image_present(frame.cmd, ci_vulkanGlobal.swapchainImages[imageIndex]);

    Render::end_frame(ci_vulkanGlobal, frame.cmd, frame.presentReady, frame.queueDoneFence, frame.imageAquired, imageIndex);

    ci_currentFrame++;
}
float counter = 0;
void  VulkanEngine::run() {
    ZoneScoped;
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
        FrameMark;
    }

    printf("Frame Average: %ld\n", ci_currentFrame / ((Uint64)((float)SDL_GetTicks64() / 1000)));
}