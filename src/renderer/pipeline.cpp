#include "pipeline.h"
#include "mesh.h"
#include "util/vk_initializers.h"
#include "vk_engine.h"
#include <filesystem>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace util {
    bool load_shader_module(VkDevice device, const char *filePath, VkShaderModule *outShaderModule) {
        std::string full_path = std::string(PROJECT_ROOT_PATH) + "/" + filePath;

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
        createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext                    = nullptr;

        // codeSize has to be in bytes, so multply the ints in the buffer by size of
        // int to know the real size of the buffer
        createInfo.codeSize = buffer.size() * sizeof(uint32_t);
        createInfo.pCode    = buffer.data();

        // check that the creation goes well.
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return false;
        }
        *outShaderModule = shaderModule;
        return true;
    }
} // namespace util
namespace vkinit {
    VkPipelineColorBlendStateCreateInfo color_blend_info(std::vector<VkPipelineColorBlendAttachmentState> colorBlendStates) {
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.pNext                               = nullptr;

        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.logicOp         = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = colorBlendStates.size();
        colorBlending.pAttachments    = colorBlendStates.data();
        return colorBlending;
    }

    VkPipelineViewportStateCreateInfo viewport_state_info(VkViewport *viewport, VkRect2D *scissor) {

        VkPipelineViewportStateCreateInfo viewportState = {};

        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = scissor;

        return viewportState;
    }

    VkPipelineMultisampleStateCreateInfo multi_sample_info(VkSampleCountFlagBits sample_count, VkBool32 shadingSample) {

        VkPipelineMultisampleStateCreateInfo multisampleInfo = {};

        multisampleInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleInfo.rasterizationSamples = sample_count;
        multisampleInfo.sampleShadingEnable  = shadingSample; // You can set this to VK_TRUE if you want to enable sample shading
    }

}; // namespace vkinit
void ShaderContext::init_all_shaders(VkDevice device) {
    std::filesystem::path shader_path = PROJECT_ROOT_PATH + std::string("/shaders");

    // Iterate over the files in the folder
    for (const auto &entry : std::filesystem::directory_iterator(shader_path)) {
        VkShaderModule module;
        util::load_shader_module(device, entry.path().c_str(), &module);
        ci_shaders[entry.path().filename()] = module;
    }
}
void Pipeline::init_pipeline(VkDevice device) { ci_shaders.init_all_shaders(device); }
void Pipeline::build_pipeline(PipelineBuilder builder, std::string name) {
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();

    layoutInfo.pSetLayouts    = builder.ci_layoutSets.data();
    layoutInfo.setLayoutCount = builder.ci_layoutSets.size();

    VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &builder._pipelineLayout));

    VkPipelineViewportStateCreateInfo    viewportState   = vkinit::viewport_state_info(&builder._viewport, &builder._scissor);
    VkPipelineColorBlendStateCreateInfo  colorBlending   = vkinit::color_blend_info({builder._colorBlendAttachment});
    VkPipelineMultisampleStateCreateInfo multisampleInfo = vkinit::multi_sample_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE);

    builder.add_shader(ci_shaders.ci_shaders[builder.ci_vertexShader], VK_SHADER_STAGE_VERTEX_BIT);
    builder.add_shader(ci_shaders.ci_shaders[builder.ci_fragmentShader], VK_SHADER_STAGE_FRAGMENT_BIT);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = &builder.ci_renderInfo;
    pipelineInfo.renderPass                   = nullptr;

    pipelineInfo.pMultisampleState = &multisampleInfo;

    pipelineInfo.stageCount          = builder._shaderStages.size();
    pipelineInfo.pStages             = builder._shaderStages.data();
    pipelineInfo.pVertexInputState   = &builder._vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &builder._inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &builder._rasterizer;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &builder._depthStencil;
    pipelineInfo.layout              = builder._pipelineLayout;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    // its easy to error out on create graphics pipeline, so we handle it a bit
    // better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipline\n";
    } else {
        pipelines[name] = PipelineContext{builder._pipelineLayout, newPipeline};
    }
}

PipelineBuilder &PipelineBuilder::add_viewport(VkExtent2D windowExtent) {
    // /*VIEWPORT*/
    // // build viewport and scissor from the swapchain extents
    _viewport.x        = 0.0f;
    _viewport.y        = 0.0f;
    _viewport.width    = (float)windowExtent.width;
    _viewport.height   = (float)windowExtent.height;
    _viewport.minDepth = 0.0f;
    _viewport.maxDepth = 1.0f;
    _scissor.offset    = {0, 0};
    _scissor.extent    = {windowExtent.width, windowExtent.height};

    return *this;
}

PipelineBuilder &PipelineBuilder::add_vertex_attribute(VertexInputDescription vertex_attributes) {
    _vertexInputInfo.pVertexAttributeDescriptions = vertex_attributes.attributes.data();
    _vertexInputInfo.pVertexBindingDescriptions   = vertex_attributes.bindings.data();

    _vertexInputInfo.vertexAttributeDescriptionCount = vertex_attributes.attributes.size();
    _vertexInputInfo.vertexBindingDescriptionCount   = vertex_attributes.bindings.size();

    return *this;
}
PipelineBuilder &PipelineBuilder::add_shader(VkShaderModule module, VkShaderStageFlagBits shaderStage) {
    _shaderStages.push_back({vkinit::pipeline_shader_stage_create_info(shaderStage, module)});
    return *this;
}

PipelineBuilder &PipelineBuilder::add_layout_sets(std::vector<VkDescriptorSetLayout> layoutSets) {
    ci_layoutSets.insert(ci_layoutSets.end(), layoutSets.begin(), layoutSets.end());
    return *this;
}

PipelineBuilder &PipelineBuilder::add_cull_mode(VkCullModeFlags cullMode) {
    _rasterizer.cullMode = cullMode;
    return *this;
}

PipelineBuilder &PipelineBuilder::add_depth(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp) {
    _depthStencil = vkinit::depth_stencil_create_info(bDepthTest, bDepthWrite, compareOp);
    return *this;
}

PipelineBuilder &PipelineBuilder::add_attachments_formats(std::vector<VkFormat> colorFormats, VkFormat depthFormat) {
    VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineCreateInfo.pNext                            = nullptr;
    pipelineCreateInfo.pColorAttachmentFormats          = colorFormats.data();
    pipelineCreateInfo.colorAttachmentCount             = colorFormats.size();
    pipelineCreateInfo.depthAttachmentFormat            = depthFormat;
    this->ci_renderInfo                                 = {pipelineCreateInfo};
    return *this;
}

PipelineBuilder PipelineBuilder::default_pipeline_builder(VulkanContextGlobal vulkanContext, std::string vertexShader, std::string fragmentShader) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._inputAssembly   = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder = pipelineBuilder.add_viewport(vulkanContext.windowExtent).add_vertex_attribute(VertexBlock::vertex_input_description());

    pipelineBuilder.ci_vertexShader   = vertexShader;
    pipelineBuilder.ci_fragmentShader = fragmentShader;

    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
    pipelineBuilder._depthStencil         = vkinit::depth_stencil_create_info(false, false, VK_COMPARE_OP_NEVER);
    pipelineBuilder._rasterizer           = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode  = VK_CULL_MODE_NONE;

    return pipelineBuilder;
}
