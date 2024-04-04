#include "pipeline.h"
#include "common/std_common.h"
void init_pipelines(std::unordered_map<std::string, VkShaderModule> &shaders) {

    std::unordered_map<std::string, VkShaderModule> shaderModules;

    PipelineBuilder pipelineBuilder;
    /*SHADERS PUSH*/
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, ci_shaderContext.ci_shaders["colored_triangle.vert.spv"]));
    pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, ci_shaderContext.ci_shaders["colored_triangle.frag.spv"]));

    /*Layout PUSH*/
    VkPipelineLayoutCreateInfo triangleLayoutInfo = vkinit::pipeline_layout_create_info();

    VkDescriptorSetLayout layouts[]   = {c_cameraLayout, c_objectLayout, c_textureLayout};
    triangleLayoutInfo.pSetLayouts    = layouts;
    triangleLayoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);

    VK_CHECK(vkCreatePipelineLayout(this->ci_vulkanGlobal.device, &triangleLayoutInfo, nullptr, &this->pipelineLayout));

    /*Pipeline Settings*/
    pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
    pipelineBuilder._pipelineLayout  = this->pipelineLayout;

    pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // /*VIEWPORT*/
    // // build viewport and scissor from the swapchain extents
    pipelineBuilder._viewport.x        = 0.0f;
    pipelineBuilder._viewport.y        = 0.0f;
    pipelineBuilder._viewport.width    = (float)ci_vulkanGlobal.windowExtent.width;
    pipelineBuilder._viewport.height   = (float)ci_vulkanGlobal.windowExtent.height;
    pipelineBuilder._viewport.minDepth = 0.0f;
    pipelineBuilder._viewport.maxDepth = 1.0f;

    pipelineBuilder._scissor.offset = {0, 0};
    pipelineBuilder._scissor.extent = ci_vulkanGlobal.windowExtent;

    // /*Extra*/VkCullModeFlagBits
    // // configure the rasterizer to draw filled triangles
    pipelineBuilder._rasterizer          = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    pipelineBuilder._rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

    // // a single blend attachment with no blending and writing to RGBA
    pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

    // // default depthtesting
    pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // /*Vertex Bindings*/

    auto description = vertex_input_description();

    pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = description.attributes.data();
    pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions   = description.bindings.data();

    pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = description.attributes.size();
    pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount   = description.bindings.size();

    VkPipelineRenderingCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineCreateInfo.pNext                            = nullptr;
    pipelineCreateInfo.pColorAttachmentFormats          = &this->_swapchainImageFormat;
    pipelineCreateInfo.colorAttachmentCount             = 1;
    pipelineCreateInfo.depthAttachmentFormat            = _depthFormat;

    this->pipeline = pipelineBuilder.build_pipeline(ci_vulkanGlobal.device, pipelineCreateInfo);
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkPipelineRenderingCreateInfoKHR pass) {
    // make viewport state from our stored viewport and scissor.
    // at the moment we wont support multiple viewports or scissors

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &_viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &_scissor;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &_colorBlendAttachment;

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this
    // one to create the pipeline

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.flags                                = 0;
    multisampleInfo.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable                  = VK_FALSE; // You can set this to VK_TRUE if you want to enable sample shading
    multisampleInfo.pNext                                = nullptr;

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