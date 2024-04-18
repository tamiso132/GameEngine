#pragma once

#include "util/mesh.h"
#include "util/pipeline.h"
#include "util/vk_resource.h"
#include "util/vk_types.h"
#include <cstdint>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

struct GuiWindow {
    uint32_t xPos{};
    uint32_t yPos{};

    uint32_t width{};
    uint32_t height{};
};

struct TGui {

    DescriptorAllocatorGrowable ci_allocator;
    VkDescriptorSetLayout       ci_layout;
    PipelineContext             ci_pipeline;
    ResourceContext            *ci_resource;
    Mesh                        ci_meshes;

    GPUObject transform(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {

        glm::mat4 trans = glm::mat4(1.0f);

        trans = glm::translate(trans, glm::vec3(x, y, 0.0f));
        trans = glm::scale(trans, glm::vec3(width, height, 0.0f));

        return GPUObject{trans};
    };

    void init(VulkanContextGlobal &vulkanContext, ResourceContext &resource, Pipeline &pipeline, Mesh &mesh) {

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
        ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});
        ci_allocator.init(vulkanContext.device, 10, ratios);

        std::vector<GPUObject> objects{};
        objects.push_back(transform(0, 0, 10, 10));
        objects.push_back(transform(50, 50, 10, 20));
        objects.push_back(transform(200, 200, 10, 30));

        ci_resource = &resource;
        ci_meshes   = mesh;

        /*Resource*/
        ResourceBuilder resourceBuilder;
        resourceBuilder.add_buffer(sizeof(GPUObject) * 3, BufferType::Storage, "2D_GUI_OBJECT", MemoryType::HostVisible, objects.data(), sizeof(GPUObject) * 3);
        ResourceIndex objectBuffer = resource.build_resource_blueprint(resourceBuilder.get_blueprint());

        /*layout*/
        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.add_buffer(objectBuffer, VK_SHADER_STAGE_VERTEX_BIT);
        ci_layout = resource.build_descriptor_layout_blueprint(layoutBuilder.get_blueprint(), ci_allocator);

        /*Pipeline*/
        PipelineBuilder pipelineBuilder = PipelineBuilder::default_pipeline_builder(vulkanContext, "gui.vert.spv", "gui.frag.spv");
        pipelineBuilder.add_vertex_attribute(Vertex2DGui::vertex_input_description())
            .add_attachments_formats(vulkanContext.swapchainImageFormat, vulkanContext.depthImage.format)
            .add_viewport(vulkanContext.windowExtent)
            .add_layout_sets({ci_layout});

        pipeline.build_pipeline(pipelineBuilder, "gui_layout");
        ci_pipeline = pipeline.get_pipeline_layout("gui_layout");
    }

    void draw_gui(VkCommandBuffer cmd) {

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ci_pipeline.pipeline);
        VkDescriptorSet c = ci_resource->get_descriptor_sets_by_layout(ci_layout).sets[0];

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ci_pipeline.layout, 0, 1, &c, 0, nullptr);
        auto               mesh   = ci_meshes.get_mesh(MeshType::GUI2DQuad);
        const VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.verticeBuffer._buffer, &offset);

        for (int i = 0; i < 3; i++) {
            vkCmdDraw(cmd, mesh.verticeSize, 1, 0, i);
        }
    }
};
