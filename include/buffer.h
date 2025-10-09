#pragma once

#include <vulkan/vulkan.hpp>
#include "camera.h"
#include "quad.h"
#include "light.h"
#include "model.h"

class BulkinBuffer {
public:
  vk::Buffer quadVertexBuffer;
  vk::Buffer quadIndexBuffer;
  std::vector<vk::Buffer> modelVertexBuffers;
  std::vector<vk::Buffer> modelIndexBuffers;
  std::vector<vk::Buffer> uniformBuffers;
  vk::Buffer ssboBuffer;
  vk::Buffer pointLightBuffer;
  
  static void createBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
  void createBuffers(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, BulkinQuad quad, std::vector<PointLight>& pointLights, std::vector<BulkinModel>& models);
  void updateUniformBuffer(uint32_t currentImage, float width, float height, BulkinCamera& camera);
  void cleanup(vk::Device& device);
  static uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice device);
  static vk::CommandBuffer beginSingleTimeCommands(vk::Device device, vk::CommandPool& commandPool);
  static void endSingleTimeCommands(vk::CommandBuffer commandBuffer, vk::Device device, vk::Queue graphicsQueue, vk::CommandPool commandPool);
  static void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandPool& commandPool, vk::Device& device, vk::Queue& queue);
private:
  vk::DeviceMemory vertexBufferMemory;
  vk::DeviceMemory indexBufferMemory;
  std::vector<vk::DeviceMemory> modelVertexBuffersMemory;
  std::vector<vk::DeviceMemory> modelIndexBuffersMemory;
  std::vector<vk::DeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  vk::DeviceMemory ssboBufferMemory;
  vk::DeviceMemory pointLightBufferMemory;
  
  void createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DeviceSize size, std::vector<Vertex> vertices, vk::Buffer &buffer, vk::DeviceMemory& bufferMemory);
  void createIndexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, vk::DeviceSize size, std::vector<uint32_t> indices, vk::Buffer &buffer, vk::DeviceMemory& bufferMemory);
  void createSSBOBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue, BulkinQuad quad, std::vector<BulkinModel>& models);
  void createUniformBuffers(vk::Device& device, vk::PhysicalDevice& physicalDevice);
  void createPointLightBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool &commandPool, vk::Queue &graphicsQueue, std::vector<PointLight>& pointLights);
};
