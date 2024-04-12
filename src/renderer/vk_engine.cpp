#include "vk_engine.h"

#define NDebug
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

const uint32_t MAX_OBJECTS = 20;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

PFN_vkCmdBeginRenderingKHR cmdBeginRenderingKHR;
PFN_vkCmdEndRenderingKHR   cmdEndRenderingKHR;

// Initilizes all the basic things for renderer

void VulkanEngine::init() {
    // We initialize SDL and create a window with it.

    vkb::DispatchTable table;

    Render::init(ci_vulkanGlobal, _allocator, table, ci_resourceContext, ci_meshes);

    cmdBeginRenderingKHR = table.fp_vkCmdBeginRenderingKHR;
    cmdEndRenderingKHR   = table.fp_vkCmdEndRenderingKHR;

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

    VkDescriptorSetLayout layout = nullptr;

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
    ratios.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
    ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});

    ci_globalDescriptorAllocator.init(ci_vulkanGlobal.device, 5, ratios);

    DescriptorLayoutBuilder textureLayoutBuilder;
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas"));
    textureLayoutBuilder.add_image(ci_resourceContext.get_resource_by_name("texture_atlas_normal"));
    textureLayoutBuilder.add_buffer(ci_resourceContext.get_resource_by_name("materials"), VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout textureLayout = ci_resourceContext.build_descriptor_layout_blueprint(textureLayoutBuilder.get_blueprint(), ci_globalDescriptorAllocator);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

        char c;
        if (i == 0) {
            c = '\0';
        } else {
            c = static_cast<char>(i - 1);
        }

        frame.bufferIndex = ci_resourceContext.build_resource_blueprint(dynamicBuilder.get_blueprint());

        auto cameraIndex  = ci_resourceContext.get_resource_by_name("camera" + c);
        auto objectsIndex = ci_resourceContext.get_resource_by_name("objects" + c);

        DescriptorLayoutBuilder layoutBuilder;

        layoutBuilder.add_buffer(cameraIndex, VK_SHADER_STAGE_VERTEX_BIT);
        layoutBuilder.add_buffer(objectsIndex, VK_SHADER_STAGE_VERTEX_BIT);

        layout = ci_resourceContext.build_descriptor_layout_blueprint(layoutBuilder.get_blueprint(), frame.descriptorAllocator, layout);

        ci_frames.push_back(frame);
    };

    /*Pipeline Stuff*/

    pipeline.init_pipeline(ci_vulkanGlobal.device);

    PipelineBuilder pipelineBuilder = PipelineBuilder::default_pipeline_builder(ci_vulkanGlobal, "colored_triangle.vert.spv", "colored_triangle.frag.spv");

    pipelineBuilder.add_attachments_formats({ci_vulkanGlobal.swapchainImageFormat}, ci_vulkanGlobal.depthImage.format);
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
    FrameData frame = ci_frames[ci_currentFrame % ci_frames.size()];

    if (SDL_GetWindowFlags(ci_vulkanGlobal.window) & SDL_WINDOW_MINIMIZED)
        return;

    uint32_t imageIndex;

    Render::prepare_frame(ci_vulkanGlobal, frame.cmd, frame.queueDoneFence, frame.imageAquired, imageIndex);

    VkClearValue clearValue;
    clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f};
    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkRenderingAttachmentInfo colorAttachment =
        vkinit::render_attachment_info(clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR, ci_vulkanGlobal.swapchainImageViews[imageIndex], VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    VkRenderingAttachmentInfo depthAttachment =
        vkinit::render_attachment_info(depthClear, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ci_vulkanGlobal.depthImage.view, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    VkRenderingInfoKHR dynamicInfo = vkinit::render_info_khr(colorAttachment, depthAttachment, ci_vulkanGlobal.windowExtent);

    vkutil::transition_image_color_attachment(frame.cmd, ci_vulkanGlobal.swapchainImages[imageIndex]);

    cmdBeginRenderingKHR(frame.cmd, &dynamicInfo);

    // draw_test();

    cmdEndRenderingKHR(frame.cmd);

    vkutil::transition_image_present(frame.cmd, ci_vulkanGlobal.swapchainImages[imageIndex]);

    Render::end_frame(ci_vulkanGlobal, frame.cmd, frame.presentReady, frame.queueDoneFence, frame.imageAquired, imageIndex);

    ci_currentFrame++;
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