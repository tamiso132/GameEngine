#include <cstdint>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <iostream>
// split from a 2d perspective, until there is x,z amount of blocks.
// Then have it be an array and the up/down depends on indices of the array. the
// smallest index being the lowest one.

// for later, maybe dynamically load more chunks as player moves through the
// world

// Split the world in x,z then have an array of "height"

// should not load every chunk, have chunks saved in storage/disk

const uint32_t NODE_OBJECT_LIMIT = 1000;
const uint32_t CHUNK_SIZE = 16; // might increase later

// might use smaller variable for size
using namespace std;
struct QuadNode;

void split_node(QuadNode &node);

template <typename T> struct DynamicArray {
  T *data;
  size_t size;
};

struct NodeEnd {
  uint32_t length;
  uint32_t height;

  std::vector<uint32_t> objectIndexes;
};
struct QuadNode {
  uint32_t length;

  uint32_t xStart;
  uint32_t zStart;

  std::vector<uint32_t> objectIndexes;

  QuadNode *top_right;
  QuadNode *top_left;

  QuadNode *bottom_left;
  QuadNode *bottom_right;

  QuadNode(uint32_t length, uint32_t xStart, uint32_t zStart) {
    this->length = length;

    this->xStart = xStart;
    this->zStart = zStart;

    if (length > 16) {
      split_node(*this);
    }
  };
  QuadNode(){};
};
QuadNode root;

unordered_map<uint32_t, std::vector<uint32_t>> chunkObjects;

void init_tree(uint32_t chunkGroup) {
  root = QuadNode(chunkGroup * 4 * CHUNK_SIZE, 0, 0);
}

void insert_object(uint32_t test, int x, int y) {}

void split_node(QuadNode &node) {
  uint32_t newLength = node.length / 2;

  uint32_t rightX = (node.length - newLength) + node.xStart;
  uint32_t bottomZ = node.zStart + newLength;

  uint32_t rightLength = node.length - newLength;

  node.top_left = new QuadNode(newLength, node.xStart, node.zStart);

  node.top_right = new QuadNode(rightLength, rightX, node.zStart);

  node.bottom_left = new QuadNode(newLength, node.xStart, bottomZ);

  node.bottom_right = new QuadNode(rightLength, rightX, bottomZ);
}

void init_world(uint32_t chunkFourMultitude) {
  std::ofstream outputFile("worldData/chunks", ios::app);
}