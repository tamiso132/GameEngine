#include "material_editor.h"
#include <cstdio>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

int main(int argc, char *argv[]) {
    MaterialEditor engine{};
    engine.init();
    
    engine.run();
}