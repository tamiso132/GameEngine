#include "../util/helper.h"
#include "../vk_mesh.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <stb_image.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "../util/vk_initializers.h"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cube {

// const glm::vec3 vertices[] = {
//     {-0.5f, -0.5f, -0.5f}, // front_top_left
//     {0.5f, 0.5f, 0.5f},    // front_top_right
//     {-0.5f, -0.5f, 0.5f},  // front_bottom_left
//     {0.5f, -0.5f, 0.5f},   // front_bottom_right

//     {-0.5f, 0.5f, -0.5f},  // back_top_left
//     {0.5f, 0.5f, -0.5f},   // back_top_right
//     {-0.5f, -0.5f, -0.5f}, // back_bottom_left
//     {0.5f, -0.5f, -0.5f},  // back_bottom_right
// };

// const uint16_t indices[] = {
//     0, 1, 2, // front Face cube
//     2, 3, 1, // front Face cube

//     1, 5, 3, // Right Face Cube
//     3, 7, 5, // Right Face Cube

//     0, 4, 2, // Left Face Cube
//     2, 6, 4, // Left Face Cube

//     4, 5, 6, // Back Face Cube,
//     6, 7, 5, // Back Face Cube

//     4, 5, 0, // Top Face Cube
//     0, 1, 4, // Top Face Cube

//     6, 7, 2, // Bottom Face Cube
//     2, 3, 7, // Bottom Face Cube
// };

const glm::vec2 uv_dirt = {

};

}; // namespace Cube

namespace CubeMap {

const uint32_t GRID_LENGTH = 256;

static void load_cube_map(AllocatedImage *cubeMapImage, VkImageView *view) {

    std::vector<std::pair<uint32_t, uint32_t>> gridIndex = {
        // DIRT
        {3, 0},  {3, 0},  {0, 0},  {2, 0},  {3, 0},  {3, 0}, // DIRT
        {0, 1},  {0, 1},  {0, 1},  {0, 1},  {0, 1},  {0, 1}, // STONE

        {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  // COAL
        {2, 1},  {2, 1},  {2, 1},  {2, 1},  {2, 1},  {2, 1},  // IRON
        {1, 1},  {1, 1},  {1, 1},  {1, 1},  {1, 1},  {1, 1},  // GOLD
        {20, 1}, {20, 1}, {20, 1}, {20, 1}, {20, 1}, {20, 1}, // REDSTONE
        {19, 1}, {19, 1}, {19, 1}, {19, 1}, {19, 1}, {19, 1}, // DIAMOND
    };

    Helper::create_cube_map("assets/lost_empire-RGBA.png", 258, gridIndex, cubeMapImage);

    VkImageViewCreateInfo viewInfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, cubeMapImage->_image, VK_IMAGE_ASPECT_COLOR_BIT);
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.subresourceRange.layerCount = 6 * gridIndex.size() / 6;
    viewInfo.image = cubeMapImage->_image;

    vkCreateImageView(Helper::device, &viewInfo, nullptr, view);
} // namespace CubeMap
} // namespace CubeMap