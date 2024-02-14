#include "collision/octrees.h"
#include "vk_engine.h"
int main(int argc, char *argv[]) {
  VulkanEngine engine;

  init_tree(2000);

  engine.init();

  engine.run();

  engine.cleanup();
  return 0;
}
