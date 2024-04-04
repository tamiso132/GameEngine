#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

class PipelineContext {};
class PipelineBuilder {
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
    VkPipeline                                   build_pipeline(VkDevice device, VkPipelineRenderingCreateInfoKHR renderInfo);
};
