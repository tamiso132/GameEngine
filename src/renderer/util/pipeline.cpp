#include "pipeline.h"
#include "vk_initializers.h"
#include "vkutil.h"

#include <filesystem>
#include <fstream>

void ShaderContext::init_all_shaders(VkDevice device) {
    auto relativeShaderPath = std::string("/shaders/spiv");

    std::filesystem::path shader_path = PROJECT_ROOT_PATH + relativeShaderPath;

    // Iterate over the files in the folder
    for (const auto &entry : std::filesystem::directory_iterator(shader_path)) {

        if (entry.path().filename().extension() != ".spv") {
            printf("extension: %s\n", entry.path().filename().extension().c_str());
            continue;
        }

        VkShaderModule module        = nullptr;
        std::string    relative_path = relativeShaderPath + "/" + entry.path().filename().c_str();
        vkutil::load_shader_module(device, relative_path.c_str(), &module);

        if (module == nullptr) {
            printf("Error reading shader file. %s\n", entry.path().c_str());
        } else {

            ci_shaders[entry.path().filename()] = module;
            vkutil::debug_object_set_name((uint64_t)module, VK_OBJECT_TYPE_SHADER_MODULE, entry.path().filename().c_str(), device);
        }
    }
}
void Pipeline::init_pipeline(VkDevice device) {
    ci_shaders.init_all_shaders(device);
    this->device = device;
}

void Pipeline::build_pipeline(PipelineBuilder builder, std::string name) {
    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();

    layoutInfo.pSetLayouts    = builder.ci_layoutSets.data();
    layoutInfo.setLayoutCount = builder.ci_layoutSets.size();
    VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &builder._pipelineLayout));

    VkPipelineViewportStateCreateInfo viewportState = vkinit::viewport_state_info(&builder._viewport, &builder._scissor);

    VkPipelineMultisampleStateCreateInfo multisampleInfo = vkinit::multi_sample_info(VK_SAMPLE_COUNT_1_BIT, VK_FALSE);

    builder.add_shader(ci_shaders.ci_shaders[builder.ci_vertexShader], VK_SHADER_STAGE_VERTEX_BIT);
    builder.add_shader(ci_shaders.ci_shaders[builder.ci_fragmentShader], VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &builder._colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = &builder.ci_renderInfo;
    pipelineInfo.renderPass                   = nullptr;

    pipelineInfo.pMultisampleState = &multisampleInfo;

    pipelineInfo.stageCount = builder._shaderStages.size();
    pipelineInfo.pStages    = builder._shaderStages.data();

    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &builder._rasterizer;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &builder._depthStencil;
    pipelineInfo.layout              = builder._pipelineLayout;
    pipelineInfo.pInputAssemblyState = &builder._inputAssembly;
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    VkPipelineVertexInputStateCreateInfo vertexInputState = vkinit::vertex_input_state_create_info();

    vertexInputState.vertexAttributeDescriptionCount = builder.ci_vertexInput.attributes.size();
    vertexInputState.vertexBindingDescriptionCount   = builder.ci_vertexInput.bindings.size();

    vertexInputState.pVertexBindingDescriptions   = builder.ci_vertexInput.bindings.data();
    vertexInputState.pVertexAttributeDescriptions = builder.ci_vertexInput.attributes.data();

    pipelineInfo.pVertexInputState = &vertexInputState;
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
    ci_vertexInput = vertex_attributes;
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

PipelineBuilder &PipelineBuilder::add_attachments_formats(VkFormat format, VkFormat depthFormat) {
    ci_swapchainFormat = format;
    ci_renderInfo      = vkinit::pipeline_render_create_info_khr(ci_swapchainFormat, depthFormat);
    return *this;
}

PipelineBuilder PipelineBuilder::default_pipeline_builder(VulkanContextGlobal vulkanContext, std::string vertexShader, std::string fragmentShader) {
    PipelineBuilder pipelineBuilder;
    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder = pipelineBuilder.add_viewport(vulkanContext.windowExtent).add_vertex_attribute(VertexBlock::vertex_input_description());

    pipelineBuilder.ci_vertexShader   = vertexShader;
    pipelineBuilder.ci_fragmentShader = fragmentShader;

    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
    pipelineBuilder._depthStencil         = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    pipelineBuilder._rasterizer           = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode  = VK_CULL_MODE_NONE;

    return pipelineBuilder;
}
