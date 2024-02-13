
#include "helper.h"
#include "vk_types.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <vk_mem_alloc.h>

VkDevice Helper::device;
VkPhysicalDeviceProperties Helper::gpuProperties;
VmaAllocator Helper::allocator;

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

AllocatedBuffer *Helper::create_buffer(size_t allocSize,
                                       VkBufferUsageFlags usage,
                                       VmaMemoryUsage memoryUsage) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.size = allocSize;

  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;

  AllocatedBuffer *newBuffer = new AllocatedBuffer{};

  VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo,
                           &newBuffer->_buffer, &newBuffer->_allocation,
                           nullptr));

  return newBuffer;
}

bool Helper::load_shader_module(const char *filePath,
                                VkShaderModule *outShaderModule) {
  std::string full_path = std::string(PROJECT_ROOT_PATH) + "/" + filePath;

  // open the file. With cursor at the end
  std::ifstream file(full_path, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  // find what the size of the file is by looking up the location of the cursor
  // because the cursor is at the end, it gives the size directly in bytes
  size_t fileSize = (size_t)file.tellg();

  // spirv expects the buffer to be on uint32, so make sure to reserve a int
  // vector big enough for the entire file
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  // put file cursor at beggining
  file.seekg(0);

  // load the entire file into the buffer
  file.read((char *)buffer.data(), fileSize);

  // now that the file is loaded into the buffer, we can close it
  file.close();

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;

  // codeSize has to be in bytes, so multply the ints in the buffer by size of
  // int to know the real size of the buffer
  createInfo.codeSize = buffer.size() * sizeof(uint32_t);
  createInfo.pCode = buffer.data();

  // check that the creation goes well.
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(Helper::device, &createInfo, nullptr,
                           &shaderModule) != VK_SUCCESS) {
    return false;
  }
  *outShaderModule = shaderModule;
  return true;
}

std::string Helper::get_filename_from_path(const char *path) {

  const char slash = '/';
  int index = 0;
  for (int i = 0; i < strlen(path); i++) {
    if (path[i] == slash) {
      index = i;
    }
  }
  index++;

  std::string filename;
  for (int i = 0; i < strlen(path) - index; i++) {
    filename.push_back(path[index + i]);
  }
  printf("filename: %s\n", filename.c_str());
  return filename;
}
