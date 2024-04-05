#pragma once

#include "common/std_common.h"
#include "mesh.h"
#include "resource.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

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

  private:
    ShaderContext ci_shaders;
    VkDevice      device;

    std::unordered_map<std::string, PipelineContext> pipelines;
};

class PipelineBuilder {
    PipelineBuilder(){};

  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineVertexInputStateCreateInfo         _vertexInputInfo;
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

    std::string ci_vertexShader;
    std::string ci_fragmentShader;

    PipelineBuilder &add_viewport(VkExtent2D viewport);
    PipelineBuilder &add_vertex_attribute(VertexInputDescription vertex_attributes);
    PipelineBuilder &add_shader(VkShaderModule module, VkShaderStageFlagBits shaderStage);
    PipelineBuilder &add_layout_sets(std::vector<VkDescriptorSetLayout> layoutSets);
    PipelineBuilder &add_depth(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
    PipelineBuilder &add_cull_mode(VkCullModeFlags cullMode);
    PipelineBuilder &add_attachments_formats(std::vector<VkFormat> colorFormats, VkFormat depthFormat);

    static PipelineBuilder default_pipeline_builder(VulkanContextGlobal vulkanContext, std::string vertexShader, std::string fragmentShader);
};
