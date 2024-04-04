#include "pipeline.h"
#include "mesh.h"
#include "resource.h"
#include "util/vk_initializers.h"
#include "vk_engine.h"
#include <filesystem>
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
    _scissor.extent    = windowExtent;
}

PipelineBuilder &PipelineBuilder::add_vertex_attribute(VertexInputDescription vertex_attributes) {
    _vertexInputInfo.pVertexAttributeDescriptions = vertex_attributes.attributes.data();
    _vertexInputInfo.pVertexBindingDescriptions   = vertex_attributes.bindings.data();

    _vertexInputInfo.vertexAttributeDescriptionCount = vertex_attributes.attributes.size();
    _vertexInputInfo.vertexBindingDescriptionCount   = vertex_attributes.bindings.size();
}

PipelineBuilder &PipelineBuilder::add_shader(VkShaderModule module, VkShaderStageFlagBits shaderStage) { _shaderStages.push_back({vkinit::pipeline_shader_stage_create_info(shaderStage, module)}); }
PipelineBuilder &PipelineBuilder::add_layout_sets(std::vector<VkDescriptorSetLayout> layoutSets) { ci_layoutSets.insert(ci_layoutSets.end(), layoutSets.begin(), layoutSets.end()); }
PipelineBuilder &PipelineBuilder::add_cull_mode(VkCullModeFlags cullMode) { _rasterizer.cullMode = cullMode; }
void             Pipeline::build_pipeline(PipelineBuilder builder, VkDevice device, VkPipelineRenderingCreateInfoKHR renderInfo, std::string name) {
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();

    layoutInfo.pSetLayouts    = builder.ci_layoutSets.data();
    layoutInfo.setLayoutCount = builder.ci_layoutSets.size();

    VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &builder._pipelineLayout));

    VkPipelineViewportStateCreateInfo    viewportState   = vkinit::viewport_state_info(&builder._viewport, &builder._scissor);
    VkPipelineColorBlendStateCreateInfo  colorBlending   = vkinit::color_blend_info({builder._colorBlendAttachment});
    VkPipelineMultisampleStateCreateInfo multisampleInfo = vkinit::multi_sample_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE);

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
void Pipeline::init_pipelines(const ResourceContext &context, const VulkanContextGlobal vulkanContext) {

    // // /*Extra*/VkCullModeFlagBits
    // // // configure the rasterizer to draw filled triangles
    // pipelineBuilder._rasterizer          = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    // pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    // // // a single blend attachment with no blending and writing to RGBA
    // pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // // // default depthtesting
    // pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // // /*Vertex Bindings*/
    // auto description = vertex_input_description();

    // pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = description.attributes.data();
    // pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions   = description.bindings.data();

    // pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = description.attributes.size();
    // pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount   = description.bindings.size();

    // VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
    // pipelineCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    // pipelineCreateInfo.pNext                            = nullptr;
    // pipelineCreateInfo.pColorAttachmentFormats          = &this->_swapchainImageFormat;
    // pipelineCreateInfo.colorAttachmentCount             = 1;
    // pipelineCreateInfo.depthAttachmentFormat            = _depthFormat;

    // this->pipeline = pipelineBuilder.build_pipeline(ci_vulkanGlobal.device, pipelineCreateInfo);
}

PipelineBuilder PipelineBuilder::default_pipeline_builder(VulkanContextGlobal vulkanContext, VkShaderModule vertexShader, VkShaderModule fragmentShader) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._inputAssembly   = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder.add_viewport(vulkanContext.windowExtent);
    pipelineBuilder.add_vertex_attribute(VertexBlock::vertex_input_description());
    pipelineBuilder.add_shader(vertexShader, VK_SHADER_STAGE_VERTEX_BIT);
    pipelineBuilder.add_shader(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);

    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(false, false, VK_COMPARE_OP_NEVER);

    pipelineBuilder._rasterizer          = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_NONE;

    return pipelineBuilder;
}

VkPipeline PipelineBuilder::build_pipeline(PipelineBuilder builder, VkDevice device, VkPipelineRenderingCreateInfoKHR pass) {
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();

    layoutInfo.pSetLayouts    = ci_layoutSets.data();
    layoutInfo.setLayoutCount = ci_layoutSets.size();

    VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &_pipelineLayout));

    VkPipelineViewportStateCreateInfo    viewportState   = vkinit::viewport_state_info(&_viewport, &_scissor);
    VkPipelineColorBlendStateCreateInfo  colorBlending   = vkinit::color_blend_info({_colorBlendAttachment});
    VkPipelineMultisampleStateCreateInfo multisampleInfo = vkinit::multi_sample_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = &pass;
    pipelineInfo.renderPass                   = nullptr;

    pipelineInfo.pMultisampleState = &multisampleInfo;

    pipelineInfo.stageCount          = _shaderStages.size();
    pipelineInfo.pStages             = _shaderStages.data();
    pipelineInfo.pVertexInputState   = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &_depthStencil;
    pipelineInfo.layout              = _pipelineLayout;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

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