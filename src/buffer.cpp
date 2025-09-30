#include "buffer.h"
#include "vertex.h"
#include "constants.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

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
  
  void* newData = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(newData, quadVertices.data(), size);
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
  
  void* newData = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(newData, quadIndices.data(), size);
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

void BulkinBuffer::updateUniformBuffer(uint32_t currentImage, float width, float height, BulkinCamera& camera) {
  UniformBufferObject ubo{};
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, glm::vec3(0.0f));
  model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f));
  model = glm::scale(model, glm::vec3(1.0f));
  ubo.model = model;
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
  device.destroy(vertexBuffer);
  device.free(vertexBufferMemory);
  device.destroy(indexBuffer);
  device.free(indexBufferMemory);
}
