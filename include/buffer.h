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
private:
  void createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
  vk::DeviceMemory vertexBufferMemory;
  vk::DeviceMemory indexBufferMemory;
  vk::DeviceMemory ssboBufferMemory;
};
