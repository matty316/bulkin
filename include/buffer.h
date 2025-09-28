#pragma once

#include <vulkan/vulkan.hpp>

class BulkinBuffer {
public:
  vk::Buffer vertexBuffer;
  vk::Buffer indexBuffer;

  void createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void createIndexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void cleanup(vk::Device& device);
private:
  void createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
  vk::DeviceMemory vertexBufferMemory;
  vk::DeviceMemory indexBufferMemory;
};
