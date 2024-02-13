#include <cstdint>
#include <vector>

const uint32_t NODE_OBJECT_LIMIT = 1000;
const uint32_t SPLIT_INTO_BLOCKS = 16; // might increase later
struct Node {
  uint32_t depth;
  uint32_t length;
  uint32_t height;

  uint32_t xStart;
  uint32_t yStart;
  uint32_t zStart;

  std::vector<uint32_t> objectIndexes;

  Node *height_up;
  Node *height_down;

  Node *top_right;
  Node *top_left;

  Node *bottom_left;
  Node *bottom_right;
  Node(uint32_t depth, uint32_t length, uint32_t height, uint32_t xStart,
       uint32_t yStart, uint32_t zStart) {
    this->height = height;
    this->depth = depth;
    this->length = length;

    this->xStart = xStart;
    this->yStart = yStart;
    this->zStart = zStart;
  }
  Node(){};
};

Node root;
void init_tree() { static uint32_t split = SPLIT_INTO_BLOCKS; }
void insert_object(uint32_t test, int x, int y) {}

void split_tree(Node &node) {
  node.height_up = new Node();
  node.height_down = new Node();
  node.bottom_left = new Node();
  node.bottom_right = new Node();

  uint32_t newheight = node.height / 2;

  uint32_t startYDown = node.yStart + newheight + 1;

  node.height_up = new Node(node.depth, node.length, newheight, node.xStart,
                            node.yStart, node.zStart);

  node.height_down = new Node(node.depth, node.length, newheight, node.xStart,
                              startYDown, node.zStart);

  uint32_t newDepth = node.depth / 2;
  uint32_t newLength = node.length / 2;

  node.top_left = new Node(newDepth, newLength, node.height)
}