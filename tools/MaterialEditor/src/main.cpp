#include "material_editor.h"
#include <cstdio>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

int main(int argc, char *argv[]) {
    MaterialEditor engine{};
    engine.init();
    engine.run();
}