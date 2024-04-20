#include "renderer.h"
#include "VkBootstrap.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include "vkutil.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <any>
#include <memory>
#include <vulkan/vulkan_core.h>

PFN_vkSetDebugUtilsObjectNameEXT vkutil::cmdSetDebugUtilsObjectNameEXT;

static const std::vector<const char *> device_extensions = {"VK_KHR_dynamic_rendering"};
namespace Render {
    void init(VulkanContextGlobal &vulkanContext, VmaAllocator &allocator, vkb::DispatchTable &table, ResourceContext &resourceContext, Mesh &meshes, void *newFeatures) {
        vulkanContext = {};

        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

        vulkanContext.window = SDL_CreateWindow("Vulkan Engine", 0, 0, vulkanContext.windowExtent.width, vulkanContext.windowExtent.height, window_flags);

        vkb::InstanceBuilder builder;
        // make the vulkan instance, with basic debug features
        auto inst_ret = builder
                            .set_app_name("Example Vulkan Application")

                            .require_api_version(1, 3, 0)
                            .enable_extension("VK_EXT_debug_utils")
                            .request_validation_layers(true)
                            .use_default_debug_messenger()
                            .build();

        vkb::Instance vkb_inst = inst_ret.value();

        // grab the instance
        vulkanContext.instance        = vkb_inst.instance;
        vulkanContext.debug_messenger = vkb_inst.debug_messenger;

        SDL_Vulkan_CreateSurface(vulkanContext.window, vulkanContext.instance, &vulkanContext.surface);

        VkPhysicalDeviceFeatures features;

        features.imageCubeArray    = true;
        features.samplerAnisotropy = true;
        VkPhysicalDeviceVulkan11Features features_11;
        features_11.shaderDrawParameters = true;

        VkPhysicalDeviceVulkan13Features features_13;
        features_13.dynamicRendering = true;

        VkPhysicalDeviceVulkan12Features features_12;
        features_12.bufferDeviceAddress = true;

        vkb::PhysicalDeviceSelector selector{vkb_inst};
        vkb::PhysicalDevice         physicalDevice = selector.set_minimum_version(1, 3)
                                                 .set_required_features(features)
                                                 .set_required_features_11(features_11)
                                                 .set_required_features_12(features_12)
                                                 .set_required_features_13(features_13)
                                                 .add_required_extension(device_extensions[0])
                                                 .set_surface(vulkanContext.surface)
                                                 .select()
                                                 .value();
        // create the final vulkan device

        vkb::DeviceBuilder deviceBuilder{physicalDevice};
        vkb::Device        vkbDevice;
        if (newFeatures != nullptr) {
            vkbDevice = deviceBuilder.add_pNext(newFeatures).build().value();
        } else {
            vkbDevice = deviceBuilder.build().value();
        }

        table = vkbDevice.make_table();

        // Get the VkDevice handle used in the rest of a vulkan application
        vulkanContext.device    = vkbDevice.device;
        vulkanContext.chosenGPU = physicalDevice.physical_device;

        // use vkbootstrap to get a Graphics queue
        vulkanContext.graphicQueue.queue  = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        vulkanContext.transferQueue.queue = vkbDevice.get_queue(vkb::QueueType::transfer).value();

        vulkanContext.graphicQueue.queueIndex  = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
        vulkanContext.transferQueue.queueIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

        // initialize the memory allocator
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = vulkanContext.chosenGPU;
        allocatorInfo.device                 = vulkanContext.device;
        allocatorInfo.instance               = vulkanContext.instance;
        vmaCreateAllocator(&allocatorInfo, &allocator);

        vkb::SwapchainBuilder swapchainBuilder{vulkanContext.chosenGPU, vulkanContext.device, vulkanContext.surface};

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                                          .set_desired_format({VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                          // use vsync present mode
                                          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                          .set_desired_extent(vulkanContext.windowExtent.width, vulkanContext.windowExtent.height)
                                          .build()
                                          .value();

        // store swapchain and its related images
        vulkanContext.swapchain           = vkbSwapchain.swapchain;
        vulkanContext.swapchainImages     = vkbSwapchain.get_images().value();
        vulkanContext.swapchainImageViews = vkbSwapchain.get_image_views().value();

        vulkanContext.swapchainImageFormat = vkbSwapchain.image_format;

        // depth image size will match the window
        VkExtent3D depthImageExtent = {vulkanContext.windowExtent.width, vulkanContext.windowExtent.height, 1};

        // hardcoding the depth format to 32 bit float
        vulkanContext.depthImage.format = VK_FORMAT_D32_SFLOAT;

        // the depth image will be a image with the format we selected and Depth
        // Attachment usage flag
        VkImageCreateInfo dimg_info = vkinit::image_create_info(vulkanContext.depthImage.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

        // for the depth image, we want to allocate it from gpu local memory
        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
        dimg_allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // allocate and create the image
        vmaCreateImage(allocator, &dimg_info, &dimg_allocinfo, &vulkanContext.depthImage.alloc._image, &vulkanContext.depthImage.alloc._allocation, nullptr);

        // build a image-view for the depth image to use for rendering
        VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(vulkanContext.depthImage.format, vulkanContext.depthImage.alloc._image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(vulkanContext.device, &dview_info, nullptr, &vulkanContext.depthImage.view));
        vulkanContext.pool = vkutil::create_pool(vulkanContext, vulkanContext.graphicQueue.queueIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        vulkanContext.cmd  = vkutil::create_cmd(vulkanContext, vulkanContext.graphicQueue.queueIndex, vulkanContext.pool);

        resourceContext.init(vulkanContext, table.fp_vkSetDebugUtilsObjectNameEXT);
        meshes.init_mesh(resourceContext);

        vkutil::cmdSetDebugUtilsObjectNameEXT = table.fp_vkSetDebugUtilsObjectNameEXT;
    }

    void add_imgui(VulkanContextGlobal &vulkanContext, VkDescriptorPool &imguiDescriptorPool) {

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
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets                    = 1000;
        pool_info.poolSizeCount              = (uint32_t)std::size(pool_sizes);
        pool_info.pPoolSizes                 = pool_sizes;

        VK_CHECK(vkCreateDescriptorPool(vulkanContext.device, &pool_info, nullptr, &imguiDescriptorPool));

        // 2: initialize imgui library

        // this initializes the core structures of imgui
        ImGui::CreateContext();

        // this initializes imgui for SDL
        ImGui_ImplSDL2_InitForVulkan(vulkanContext.window);

        // this initializes imgui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = vulkanContext.instance;
        init_info.PhysicalDevice            = vulkanContext.chosenGPU;
        init_info.Device                    = vulkanContext.device;
        init_info.Queue                     = vulkanContext.graphicQueue.queue;
        init_info.DescriptorPool            = imguiDescriptorPool;
        init_info.MinImageCount             = 3;
        init_info.ImageCount                = 3;
        init_info.UseDynamicRendering       = true;
        init_info.ColorAttachmentFormat     = vulkanContext.swapchainImageFormat;

        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info, VK_NULL_HANDLE);

        VkCommandBufferBeginInfo info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vkBeginCommandBuffer(vulkanContext.cmd, &info);
        ImGui_ImplVulkan_CreateFontsTexture();

        vkEndCommandBuffer(vulkanContext.cmd);

        VkSubmitInfo submitInfo = vkinit::submit_info(&vulkanContext.cmd);

        vkQueueSubmit(vulkanContext.graphicQueue.queue, 1, &submitInfo, VK_NULL_HANDLE);

        vkQueueWaitIdle(vulkanContext.graphicQueue.queue);

        // clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontsTexture();
    }

    void prepare_frame_imgui(const VulkanContextGlobal &vulkanContext) {
        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(vulkanContext.window);
        ImGui::NewFrame();
    }

    void prepare_frame(VulkanContextGlobal vulkanContext, VkCommandBuffer cmd, VkFence &queueIsDone, VkSemaphore imageIsAquired, uint32_t &imageIndex) {

        VK_CHECK(vkWaitForFences(vulkanContext.device, 1, &queueIsDone, true, 1000000000));
        VK_CHECK(vkResetFences(vulkanContext.device, 1, &queueIsDone));

        VK_CHECK(vkAcquireNextImageKHR(vulkanContext.device, vulkanContext.swapchain, 1000000000, imageIsAquired, nullptr, &imageIndex));

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    }

    void end_frame_imgui() { ImGui::Render(); }

    void end_frame(const VulkanContextGlobal &vulkanContext, VkCommandBuffer cmd, VkSemaphore &renderIsDone, VkFence &queueIsDone, VkSemaphore &imageIsAquired, uint32_t &imageIndex) {
        // /*Submit Queue*/
        VK_CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo         submit    = vkinit::submit_info(&cmd);
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        submit.pWaitDstStageMask = &waitStage;

        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores    = &imageIsAquired;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores    = &renderIsDone;

        VK_CHECK(vkQueueSubmit(vulkanContext.graphicQueue.queue, 1, &submit, queueIsDone));

        VkPresentInfoKHR presentInfo = vkinit::present_info();

        presentInfo.pSwapchains    = &vulkanContext.swapchain;
        presentInfo.swapchainCount = 1;

        presentInfo.pWaitSemaphores    = &renderIsDone;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &imageIndex;

        VK_CHECK(vkQueuePresentKHR(vulkanContext.graphicQueue.queue, &presentInfo));
    }
} // namespace Render
