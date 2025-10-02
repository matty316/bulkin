#include "buffer.h"
#include "vertex.h"
#include "constants.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice device) {
  vk::PhysicalDeviceMemoryProperties memProperties = device.getMemoryProperties();
  
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
      }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& queue) {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;
  
  vk::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());
  
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  commandBuffer.begin(beginInfo);
  
  vk::BufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = size;
  commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
  
  commandBuffer.end();
  
  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  
  if (queue.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess)
    throw std::runtime_error("failed to submit queue");
  queue.waitIdle();
  
  device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void BulkinBuffer::createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;
  
  buffer = device.createBuffer(bufferInfo);
  
  vk::MemoryRequirements memRequirments = device.getBufferMemoryRequirements(buffer);
  
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirments.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirments.memoryTypeBits,
                                             properties,
                                             physicalDevice);
  
  bufferMemory = device.allocateMemory(allocInfo);
  device.bindBufferMemory(buffer, bufferMemory, 0);
}

void BulkinBuffer::createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue) {
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  vk::DeviceSize size = sizeof(quadVertices[0]) * quadVertices.size();
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  void* data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, quadVertices.data(), size);
  device.unmapMemory(stagingBufferMemory);
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
  
  copyBuffer(stagingBuffer, vertexBuffer, size, commandPool, device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::createIndexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue) {
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  vk::DeviceSize size = sizeof(quadIndices[0]) * quadIndices.size();
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  void* data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, quadIndices.data(), size);
  device.unmapMemory(stagingBufferMemory);
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
  
  copyBuffer(stagingBuffer, indexBuffer, size, commandPool, device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::createUniformBuffers(vk::Device &device, vk::PhysicalDevice &physicalDevice) {
  vk::DeviceSize size = sizeof(UniformBufferObject);
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);
    uniformBuffersMapped[i] = device.mapMemory(uniformBuffersMemory[i], 0, size);
  }
}

void BulkinBuffer::createSSBOBuffer(vk::Device &device, vk::PhysicalDevice &physicalDevice, vk::CommandPool &commandPool, vk::Queue &graphicsQueue, Quad quad) {
  if (quad.getInstanceCount() == 0)
    std::runtime_error("no quads drawn");
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  vk::DeviceSize size = sizeof(PerInstanceData) * quad.getInstanceCount();
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  std::vector<PerInstanceData> perInstanceData;
  perInstanceData.resize(quad.getInstanceCount());
  for (size_t i = 0; i < quad.getInstanceCount(); i++) {
    perInstanceData[i].model = quad.getModelMatrix(i);
  }
  
  void* data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, perInstanceData.data(), size);
  device.unmapMemory(stagingBufferMemory);
  
  createBuffer(device, physicalDevice, size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, ssboBuffer, ssboBufferMemory);
  
  copyBuffer(stagingBuffer, ssboBuffer, size, commandPool, device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::updateUniformBuffer(uint32_t currentImage, float width, float height, BulkinCamera& camera) {
  UniformBufferObject ubo{};
  ubo.view = camera.getView();
  ubo.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
  ubo.proj[1][1] *= -1;
  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void BulkinBuffer::cleanup(vk::Device& device) {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    device.destroy(uniformBuffers[i]);
    device.free(uniformBuffersMemory[i]);
  }
  device.destroy(ssboBuffer);
  device.free(ssboBufferMemory);
  device.destroy(vertexBuffer);
  device.free(vertexBufferMemory);
  device.destroy(indexBuffer);
  device.free(indexBufferMemory);
}

void BulkinBuffer::createImageBuffer(vk::Device &device, vk::PhysicalDevice &physicalDevice, vk::CommandPool &commandPool, vk::Queue &graphicsQueue, const char* filename) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;
  
  if (!pixels)
    throw std::runtime_error("failed to load texture image");
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  
  createBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  device.unmapMemory(stagingBufferMemory);
  
  stbi_image_free(pixels);
}
