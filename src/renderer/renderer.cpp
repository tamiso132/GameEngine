#include "util/helper.h"
#include "util/vk_initializers.h"
#include "vk_engine.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "renderer.h"

void ResourceManager::create_descriptor_sets(std::string name, std::vector<DescInfo> descs) {

    for (auto desc : descs) {
        if (desc.bufferInfo) {
        }
    }
};

void update_descriptor_sets(std::string name, size_t bindingStart, size_t countBindings) {}

void ResourceManager::create_default_pipeline(std::string pipelineName, std::vector<const char *> layoutNames, std::vector<const char *> shaders, TKVertexType vertexType,
                                              VertexInputDescription vertexInputDesc, std::vector<VkFormat> colorFormats, VkFormat depthFormat) {
    PipelineBuilder pipelineBuilder;

    TKPipeline tkPipeline;

    /*SHADERS PUSH*/
    for (auto shaderName : shaders) {
        auto shader = this->shaderModules[shaderName];
        pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info((VkShaderStageFlagBits)shader.shaderType, shader.shaderModule));
    }

    /*Layout PUSH*/
    VkPipelineLayoutCreateInfo triangleLayoutInfo = vkinit::pipeline_layout_create_info();

    std::vector<VkDescriptorSetLayout> layouts;
    for (auto layoutName : layoutNames) {
        auto layout = this->descriptors[layoutName].layout;
        layouts.push_back(layout);
    }
    triangleLayoutInfo.pSetLayouts    = layouts.data();
    triangleLayoutInfo.setLayoutCount = layouts.size();

    VK_CHECK(vkCreatePipelineLayout(Helper::device, &triangleLayoutInfo, nullptr, &tkPipeline.layout));

    /*Pipeline Settings*/
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._pipelineLayout  = tkPipeline.layout;

    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // /*VIEWPORT*/
    // // build viewport and scissor from the
    // swapchain extents
    pipelineBuilder._viewport.x        = 0.0f;
    pipelineBuilder._viewport.y        = 0.0f;
    pipelineBuilder._viewport.width    = (float)windowExtent.width;
    pipelineBuilder._viewport.height   = (float)windowExtent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;

    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = windowExtent;

    pipelineBuilder._rasterizer          = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    // // a single blend attachment with no
    // blending and writing to RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // // default depthtesting
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // /*Vertex Bindings*/

    auto description = vertexInputDesc;

    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = description.attributes.data();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions   = description.bindings.data();

    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = description.attributes.size();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount   = description.bindings.size();

    VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineCreateInfo.pNext                            = nullptr;
    pipelineCreateInfo.pColorAttachmentFormats          = colorFormats.data();
    pipelineCreateInfo.colorAttachmentCount             = colorFormats.size();
    pipelineCreateInfo.depthAttachmentFormat            = depthFormat;

    tkPipeline.pipeline = pipelineBuilder.build_pipeline(Helper::device, pipelineCreateInfo);

    this->pipelines[pipelineName] = tkPipeline;
}

/*Private Functions*/
void create_default_sampler(VkSampler &sampler) {

    VkSamplerCreateInfo samplerInfo;
    samplerInfo.magFilter    = VK_FILTER_NEAREST;
    samplerInfo.minFilter    = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias   = 0.0f;
    samplerInfo.compareOp    = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.minLod       = 0.0f;
    samplerInfo.maxLod       = 1.0f;
    samplerInfo.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    vkCreateSampler(Helper::device, &samplerInfo, nullptr, &sampler);
}

void ResourceManager::load_shaders() {
    for (const auto &entry : std::filesystem::directory_iterator(shaderPath)) {
        if (std::filesystem::is_regular_file(entry.path())) {
            VkShaderModule shader;
            Helper::load_shader_module(entry.path().c_str(), &shader);
            auto c        = entry.path().c_str();
            auto filename = Helper::get_filename_from_path(c);

            TKShader tkShader;
            tkShader.shaderModule = shader;

            if (filename.find("vert")) {
                tkShader.shaderType = TKShaderStage::VERTEX;
            } else if (filename.find("frag")) {
                tkShader.shaderType = TKShaderStage::FRAGMENT;
            } else {
                std::cout << "Error with extension "
                             "type, must be either "
                             "vert or frag"
                          << std::endl;
                exit(-1);
            }

            shaderModules[filename] = tkShader;
        }
    }
}

TKShader ResourceManager::get_shader(const char *c) {
    if (!shaderModules.contains(c)) {
        printf("This shader does not exist, %s\n", c);
        exit(0);
    }
    return this->shaderModules[c];
}
