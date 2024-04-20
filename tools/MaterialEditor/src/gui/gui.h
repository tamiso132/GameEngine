#pragma once

#include "util/mesh.h"
#include "util/pipeline.h"
#include "util/vk_resource.h"
#include "util/vk_types.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Offset to advance to next glyph
};

struct GuiWindow {
    uint32_t xPos{};
    uint32_t yPos{};

    uint32_t width{};
    uint32_t height{};
};

struct GuiObject {
    glm::float32 xPos;
    glm::float32 yPos;

    glm::float32 width;
    glm::float32 height;
};

struct TGui {

    DescriptorAllocatorGrowable ci_allocator;
    VkDescriptorSetLayout       ci_layout;
    PipelineContext             ci_pipeline;
    ResourceContext            *ci_resource;
    Mesh                        ci_meshes;

    GPUObject transform(glm::float32 x, glm::float32 y, glm::float32 width, glm::float32 height) {

        glm::mat4 trans = glm::mat4(1.0f);

        trans = glm::translate(trans, glm::vec3(x, y, 0));
        trans = glm::scale(trans, glm::vec3(width, height, 1));

        return GPUObject{trans};
    };

    void load_thing() {
        /* load font file */
        long           size;
        unsigned char *fontBuffer;
        std::string    path     = std::string(PROJECT_ROOT_PATH) + "/assets/CW_BITMP.ttf";
        FILE          *fontFile = fopen(path.c_str(), "rb");
        fseek(fontFile, 0, SEEK_END);
        size = ftell(fontFile);       /* how long is the file ? */
        fseek(fontFile, 0, SEEK_SET); /* reset */

        fontBuffer = new unsigned char(size);

        fread(fontBuffer, size, 1, fontFile);
        fclose(fontFile);

        /* prepare font */
        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, fontBuffer, 0)) {
            printf("failed\n");
        }

        int b_w = 512; /* bitmap width */
        int b_h = 128; /* bitmap height */
        int l_h = 64;  /* line height */

        /* create a bitmap for the phrase */
        unsigned char *bitmap = new unsigned char(b_w * b_h);

        /* calculate font scaling */
        float scale = stbtt_ScaleForPixelHeight(&info, l_h);

        char *word = "the quick brown fox";

        int x = 0;

        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

        ascent  = roundf(ascent * scale);
        descent = roundf(descent * scale);

        int i;
        for (i = 0; i < strlen(word); ++i) {
            /* how wide is this character */
            int ax;
            int lsb;
            stbtt_GetCodepointHMetrics(&info, word[i], &ax, &lsb);
            /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */

            /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&info, word[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            /* compute y (different characters have different heights) */
            int y = ascent + c_y1;

            /* render character (stride and offset is important here) */
            int byteOffset = x + roundf(lsb * scale) + (y * b_w);
            stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, word[i]);

            /* advance x */
            x += roundf(ax * scale);

            /* add kerning */
            int kern;
            kern = stbtt_GetCodepointKernAdvance(&info, word[i], word[i + 1]);
            x += roundf(kern * scale);
        }

        /* save out a 1 channel image */
        stbi_write_png("out.png", b_w, b_h, 1, bitmap, b_w);

        /*
         Note that this example writes each character directly into the target image buffer.
         The "right thing" to do for fonts that have overlapping characters is
         MakeCodepointBitmap to a temporary buffer and then alpha blend that onto the target image.
         See the stb_truetype.h header for more info.
        */

        delete fontBuffer;
        delete bitmap;
    }

    void init(VulkanContextGlobal &vulkanContext, ResourceContext &resource, Pipeline &pipeline, Mesh &mesh) {

        load_thing();

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> ratios;
        ratios.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
        ratios.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1});
        ci_allocator.init(vulkanContext.device, 10, ratios);

        std::vector<GPUObject> objects{};
        objects.push_back(transform(0.5, 0.5, 1, 1));
        //  objects.push_back(transform(50, 50, 10, 20));
        //  objects.push_back(transform(200, 200, 10, 30));

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
        pipelineBuilder
            .add_vertex_attribute(Vertex2DGui::vertex_input_description()) // vertex description
            .add_attachments_formats(vulkanContext.swapchainImageFormat, vulkanContext.depthImage.format)
            .add_viewport(vulkanContext.windowExtent)
            .add_cull_mode(VK_CULL_MODE_NONE)
            .add_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
            .add_depth(false, false, VK_COMPARE_OP_NEVER)
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

        vkCmdDraw(cmd, mesh.verticeSize, 1, 0, 0);
    }
};
