#include "vk_mesh.h"
#include "vk_types.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

static const char *shaderPath = "shaders/spiv/";

enum class LayoutPhase { PerFrame, PerRenderPass, PerMaterial, PerObject };

enum class TKShaderStage {
    VERTEX   = VK_SHADER_STAGE_VERTEX_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
};

enum class TKBufferBindType {
    UNIFORM         = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    STORAGE         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    DYNAMIC_UNIFORM = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    DYNAMIC_STORAGE = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
};

struct DescBuffer {
    uint32_t         bindingIndex;
    size_t           bufferMaxSize;
    TKBufferBindType bindType;
    TKShaderStage    shaderType;
};

struct DescImage {
    VkSampler     sampler;
    VkImageView   view;
    VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
};

struct DescInfo {
    DescBuffer *bufferInfo;
    DescImage  *imageInfo;
};

enum class ImageBindType { COMBINED_SAMPLED = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };

enum class TKVertexType { POS_NORM_UV_FACE };

struct TKDescriptor {
    std::vector<std::optional<AllocatedBuffer>> bindingsPointers;
    VkDescriptorSet                             descriptorSet;
    VkDescriptorSetLayout                       layout;
};

struct TKPipeline {
    VkPipeline       pipeline;
    VkPipelineLayout layout;
};

struct TKShader {
    VkShaderModule shaderModule;
    TKShaderStage  shaderType;
};

class ResourceManager {
  public:
    void     load_shaders();
    TKShader get_shader(const char *c);

    void create_descriptor_sets(std::string name, std::vector<DescInfo> descs);

    void create_default_pipeline(std::string pipelineName, std::vector<const char *> layoutNames, std::vector<const char *> shaders, TKVertexType vertexType, VertexInputDescription vertexInputDesc,
                                 std::vector<VkFormat> colorFormats, VkFormat depthFormat);

  private:
    std::unordered_map<std::string, TKPipeline>   pipelines;
    std::unordered_map<std::string, TKDescriptor> descriptors;
    std::unordered_map<std::string, TKShader>     shaderModules;
    VkExtent2D                                    windowExtent;
};