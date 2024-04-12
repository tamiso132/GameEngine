
#include "util/mesh.h"
#include "util/pipeline.h"
#include "util/vk_resource.h"
#include <cstdint>

struct Gui {
    uint32_t xPos;
    uint32_t yPos;

    uint32_t width;
    uint32_t height;

    Pipeline pipeline;

    void init(ResourceContext &resource) {
        
    }
};

void draw_gui();