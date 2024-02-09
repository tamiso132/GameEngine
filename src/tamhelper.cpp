
#include "tamhelper.h"

#include <vk_mem_alloc.h>

VkDevice Helper::device;
VkPhysicalDeviceProperties Helper::gpuProperties;
VmaAllocator Helper::allocator;

// AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage,
//                               VmaMemoryUsage memoryUsage) {
//   VkBufferCreateInfo bufferInfo = {};
//   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//   bufferInfo.pNext = nullptr;
//   bufferInfo.size = allocSize;

//   bufferInfo.usage = usage;

//   VmaAllocationCreateInfo vmaallocInfo = {};
//   vmaallocInfo.usage = memoryUsage;

//   AllocatedBuffer newBuffer;

//   VK_CHECK(vmaCreateBuffer(Helper::allocator, &bufferInfo, &vmaallocInfo,
//                            &newBuffer._buffer, &newBuffer._allocation,
//                            nullptr));

//   return newBuffer;
// }

// void VulkanEngine::upload_mesh(Mesh &mesh) {
//   const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);
//   allocate vertex buffer VkBufferCreateInfo stagingBufferInfo = {};
//   stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//   stagingBufferInfo.pNext = nullptr;
//   this is the total size, in bytes,
//       of the buffer we are allocating stagingBufferInfo.size = bufferSize;
//   this buffer is going to be used as a Vertex Buffer stagingBufferInfo.usage
//   =
//       VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

//   let the VMA library know that this data should be writeable by CPU,
//       but also readable by GPU VmaAllocationCreateInfo vmaallocInfo = {};
//   vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

//   AllocatedBuffer stagingBuffer;

//   allocate the buffer @VK_CHECK(vmaCreateBuffer(
//       allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer._buffer,
//       &stagingBuffer._allocation, nullptr));

//   copy vertex data void *data;gpuPropertiesfer._allocation);

//   allocate vertex buffer VkBufferCreateInfo vertexBufferInfo = {};
//   vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//   vertexBufferInfo.pNext = nullptr;
//   this is the total size, in bytes,
//       of the buffer we are allocating vertexBufferInfo.size = bufferSize;
//   this buffer is going to be used as a Vertex Buffer vertexBufferInfo.usage =
//       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

//   let the VMA library know that this data should be gpu native
//       vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

//   allocate the buffer VK_CHECK(vmaCreateBuffer(
//       allocator, &vertexBufferInfo, &vmaallocInfo,
//       &mesh._vertexBuffer._buffer, &mesh._vertexBuffer._allocation,
//       nullptr));
//   add the destruction of triangle mesh buffer to the deletion queue
//       _mainDeletionQueue.push_function([=]() {
//         vmaDestroyBuffer(allocator, mesh._vertexBuffer._buffer,
//                          mesh._vertexBuffer._allocation);
//       });

//   immediate_submit([=](VkCommandBuffer cmd) {
//     VkBufferCopy copy;
//     copy.dstOffset = 0;
//     copy.srcOffset = 0;
//     copy.size = bufferSize;
//     vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer,
//     1,
//                     &copy);
//   });

//   vmaDestroyBuffer(allocator, stagingBuffer._buffer,
//   stagingBuffer._allocation);
// }

void Helper::init(VkDevice device, VkPhysicalDeviceProperties gpuProperties,
                  VmaAllocator allocator) {
  Helper::gpuProperties = gpuProperties;
  Helper::device = device;
  Helper::allocator = allocator;
}

size_t Helper::pad_uniform_buffer_size(size_t originalSize) {
  auto minUboAlignment =
      Helper::gpuProperties.limits.minUniformBufferOffsetAlignment;
  size_t alignedSize = originalSize;
  if (minUboAlignment > 0) {
    alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
  }
  return alignedSize;
}

AllocatedBuffer Helper::create_buffer(size_t allocSize,
                                      VkBufferUsageFlags usage,
                                      VmaMemoryUsage memoryUsage) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.size = allocSize;

  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;

  AllocatedBuffer newBuffer;

  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
                           &newBuffer._buffer, &newBuffer._allocation,
                           nullptr));

  return newBuffer;
  AllocatedBuffer b;
  return b;
}
