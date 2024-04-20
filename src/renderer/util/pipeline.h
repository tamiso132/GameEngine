#pragma once

#include "mesh.h"

class PipelineBuilder;

class ShaderContext {
  public:
    void init_all_shaders(VkDevice device);

    std::unordered_map<std::string, VkShaderModule> ci_shaders;
};
struct PipelineContext {
    VkPipelineLayout layout;
    VkPipeline       pipeline;
};

class Pipeline {
  public:
    void init_pipeline(VkDevice device);
    void build_pipeline(PipelineBuilder builder, std::string name);

    PipelineContext get_pipeline_layout(std::string name) { return pipelines[name]; }

  private:
    ShaderContext ci_shaders;
    VkDevice      device;

    std::unordered_map<std::string, PipelineContext> pipelines;
};

class PipelineBuilder {
    PipelineBuilder(){};

  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineInputAssemblyStateCreateInfo       _inputAssembly;
    VkViewport                                   _viewport;
    VkRect2D                                     _scissor;
    VkPipelineRasterizationStateCreateInfo       _rasterizer;
    VkPipelineColorBlendAttachmentState          _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo         _multisampling;
    VkPipelineLayout                             _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo        _depthStencil;
    std::vector<VkDescriptorSetLayout>           ci_layoutSets;
    VkPipelineRenderingCreateInfoKHR             ci_renderInfo;
    VkFormat                                     ci_swapchainFormat;

    std::string ci_vertexShader;
    std::string ci_fragmentShader;

    VertexInputDescription ci_vertexInput;

    PipelineBuilder &add_viewport(VkExtent2D viewport);
    PipelineBuilder &add_vertex_attribute(VertexInputDescription vertex_attributes);
    PipelineBuilder &add_shader(VkShaderModule module, VkShaderStageFlagBits shaderStage);
    PipelineBuilder &add_layout_sets(std::vector<VkDescriptorSetLayout> layoutSets);
    PipelineBuilder &add_depth(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
    PipelineBuilder &add_cull_mode(VkCullModeFlags cullMode);
    PipelineBuilder &add_attachments_formats(VkFormat colorFormats, VkFormat depthFormat);
    PipelineBuilder &add_input_assembly(VkPrimitiveTopology topology);

    static PipelineBuilder default_pipeline_builder(VulkanContextGlobal vulkanContext, std::string vertexShader, std::string fragmentShader);
};
