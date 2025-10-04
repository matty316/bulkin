#pragma once

#include <vulkan/vulkan.hpp>
#include "camera.h"
#include "quad.h"

class BulkinBuffer {
public:
  vk::Buffer vertexBuffer;
  vk::Buffer indexBuffer;
  std::vector<vk::Buffer> uniformBuffers;
  std::vector<vk::DeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  vk::Buffer ssboBuffer;

  void createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void createIndexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void createSSBOBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, Quad quad);
  void createUniformBuffers(vk::Device& device, vk::PhysicalDevice& physicalDevice);
  void updateUniformBuffer(uint32_t currentImage, float width, float height, BulkinCamera& camera);
  void cleanup(vk::Device& device);
  static void createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
  static uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice device);
  static vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool& commandPool);
  static void endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::Queue graphicsQueue, vk::CommandPool commandPool);
  static void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& queue);
private:
  vk::DeviceMemory vertexBufferMemory;
  vk::DeviceMemory indexBufferMemory;
  vk::DeviceMemory ssboBufferMemory;
};
