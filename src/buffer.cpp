#include "buffer.h"
#include "constants.h"
#include "vertex.h"

#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

uint32_t BulkinBuffer::findMemoryType(uint32_t typeFilter,
                                      vk::MemoryPropertyFlags properties,
                                      vk::PhysicalDevice device) {
  vk::PhysicalDeviceMemoryProperties memProperties =
      device.getMemoryProperties();

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void BulkinBuffer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer,
                              vk::DeviceSize size, vk::CommandPool &commandPool,
                              vk::Device &device, vk::Queue &queue) {
  auto commandBuffer = beginSingleTimeCommands(device, commandPool);

  vk::BufferCopy region{};
  region.size = size;
  commandBuffer.copyBuffer(srcBuffer, dstBuffer, region);

  endSingleTimeCommands(commandBuffer, device, queue, commandPool);
}

void BulkinBuffer::createBuffer(vk::Device &device,
                                vk::PhysicalDevice &physicalDevice, size_t size,
                                vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties,
                                vk::Buffer &buffer,
                                vk::DeviceMemory &bufferMemory) {
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  buffer = device.createBuffer(bufferInfo);

  vk::MemoryRequirements memRequirments =
      device.getBufferMemoryRequirements(buffer);

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirments.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirments.memoryTypeBits, properties, physicalDevice);

  bufferMemory = device.allocateMemory(allocInfo);
  device.bindBufferMemory(buffer, bufferMemory, 0);
}

void BulkinBuffer::createBuffers(vk::Device &device,
                                 vk::PhysicalDevice &physicalDevice,
                                 vk::CommandPool &commandPool,
                                 vk::Queue &graphicsQueue, BulkinQuad quad,
                                 std::vector<PointLight> &pointLights,
                                 std::vector<BulkinModel> &models) {
  if (quad.getInstanceCount() > 0) {
    createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue,
                       sizeof(quadVertices[0]) * quadVertices.size(),
                       quadVertices, quadVertexBuffer, vertexBufferMemory);
    createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue,
                      sizeof(quadIndices[0]) * quadIndices.size(), quadIndices,
                      quadIndexBuffer, indexBufferMemory);

    createPointLightBuffer(device, physicalDevice, commandPool, graphicsQueue,
                           pointLights);
  }
  createSSBOBuffer(device, physicalDevice, commandPool, graphicsQueue, quad,
                   models);
  createUniformBuffers(device, physicalDevice);

  modelVertexBuffers.resize(models.size());
  modelVertexBuffersMemory.resize(models.size());
  modelIndexBuffers.resize(models.size());
  modelIndexBuffersMemory.resize(models.size());
  for (size_t i = 0; i < models.size(); i++) {
    createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue,
                       sizeof(Vertex) * models[i].getVerticesSize(),
                       models[i].getVertices(), modelVertexBuffers[i],
                       modelVertexBuffersMemory[i]);
    createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue,
                      sizeof(uint32_t) * models[i].getIndicesSize(),
                      models[i].getIndices(), modelIndexBuffers[i],
                      modelIndexBuffersMemory[i]);
  }
}

void BulkinBuffer::createVertexBuffer(
    vk::Device &device, vk::PhysicalDevice &physicalDevice,
    vk::CommandPool &commandPool, vk::Queue &graphicsQueue, vk::DeviceSize size,
    std::vector<Vertex> vertices, vk::Buffer &buffer,
    vk::DeviceMemory &bufferMemory) {
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferSrc,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent,
               stagingBuffer, stagingBufferMemory);

  void *data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, vertices.data(), size);
  device.unmapMemory(stagingBufferMemory);

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eVertexBuffer,
               vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, bufferMemory);

  copyBuffer(stagingBuffer, buffer, size, commandPool, device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::createIndexBuffer(
    vk::Device &device, vk::PhysicalDevice &physicalDevice,
    vk::CommandPool &commandPool, vk::Queue &graphicsQueue, vk::DeviceSize size,
    std::vector<uint32_t> indices, vk::Buffer &buffer,
    vk::DeviceMemory &bufferMemory) {
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferSrc,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent,
               stagingBuffer, stagingBufferMemory);

  void *data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, indices.data(), size);
  device.unmapMemory(stagingBufferMemory);

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eIndexBuffer,
               vk::MemoryPropertyFlagBits::eDeviceLocal, buffer, bufferMemory);

  copyBuffer(stagingBuffer, buffer, size, commandPool, device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::createUniformBuffers(vk::Device &device,
                                        vk::PhysicalDevice &physicalDevice) {
  vk::DeviceSize size = sizeof(UniformBufferObject);
  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(device, physicalDevice, size,
                 vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 uniformBuffers[i], uniformBuffersMemory[i]);
    uniformBuffersMapped[i] =
        device.mapMemory(uniformBuffersMemory[i], 0, size);
  }
}

void BulkinBuffer::createPointLightBuffer(
    vk::Device &device, vk::PhysicalDevice &physicalDevice,
    vk::CommandPool &commandPool, vk::Queue &graphicsQueue,
    std::vector<PointLight> &pointLights) {
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  vk::DeviceSize pointLightBufferSize =
      sizeof(pointLights[0]) * pointLights.size();

  createBuffer(device, physicalDevice, pointLightBufferSize,
               vk::BufferUsageFlagBits::eTransferSrc,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent,
               stagingBuffer, stagingBufferMemory);

  void *data = device.mapMemory(stagingBufferMemory, 0, pointLightBufferSize);
  memcpy(data, pointLights.data(), pointLightBufferSize);
  device.unmapMemory(stagingBufferMemory);

  createBuffer(device, physicalDevice, pointLightBufferSize,
               vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eStorageBuffer,
               vk::MemoryPropertyFlagBits::eDeviceLocal, pointLightBuffer,
               pointLightBufferMemory);

  copyBuffer(stagingBuffer, pointLightBuffer, pointLightBufferSize, commandPool,
             device, graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::createSSBOBuffer(vk::Device &device,
                                    vk::PhysicalDevice &physicalDevice,
                                    vk::CommandPool &commandPool,
                                    vk::Queue &graphicsQueue, BulkinQuad quad,
                                    std::vector<BulkinModel> &models) {
  if (quad.getInstanceCount() == 0)
    std::runtime_error("no quads drawn");
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  vk::DeviceSize size =
      sizeof(PerInstanceData) * (quad.getInstanceCount() + models.size());

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferSrc,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent,
               stagingBuffer, stagingBufferMemory);

  std::vector<PerInstanceData> perInstanceData;
  perInstanceData.resize(quad.getInstanceCount() + models.size());
  for (size_t i = 0; i < quad.getInstanceCount(); i++) {
    perInstanceData[i] = quad.getInstanceData(i);
  }
  for (size_t i = quad.getInstanceCount();
       i < quad.getInstanceCount() + models.size(); i++) {
    perInstanceData[i] = PerInstanceData{
        .model = models[i - quad.getInstanceCount()].modelMatrix(),
        .faceId = 0,
        .textureIndex = models[i - quad.getInstanceCount()].getTextureId()};
  }

  void *data = device.mapMemory(stagingBufferMemory, 0, size);
  memcpy(data, perInstanceData.data(), size);
  device.unmapMemory(stagingBufferMemory);

  createBuffer(device, physicalDevice, size,
               vk::BufferUsageFlagBits::eTransferDst |
                   vk::BufferUsageFlagBits::eStorageBuffer,
               vk::MemoryPropertyFlagBits::eDeviceLocal, ssboBuffer,
               ssboBufferMemory);

  copyBuffer(stagingBuffer, ssboBuffer, size, commandPool, device,
             graphicsQueue);
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
}

void BulkinBuffer::updateUniformBuffer(uint32_t currentImage, float width,
                                       float height, BulkinCamera &camera) {
  UniformBufferObject ubo{};
  ubo.view = camera.getView();
  ubo.proj =
      glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
  ubo.proj[1][1] *= -1;
  ubo.viewPos = camera.getPosition();
  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void BulkinBuffer::cleanup(vk::Device &device) {
  for (size_t i = 0; i < modelVertexBuffers.size(); i++) {
    device.destroy(modelVertexBuffers[i]);
    device.destroy(modelIndexBuffers[i]);
    device.free(modelVertexBuffersMemory[i]);
    device.free(modelIndexBuffersMemory[i]);
  }
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    device.destroy(uniformBuffers[i]);
    device.free(uniformBuffersMemory[i]);
  }
  device.destroy(pointLightBuffer);
  device.free(pointLightBufferMemory);
  device.destroy(ssboBuffer);
  device.free(ssboBufferMemory);
  device.destroy(quadVertexBuffer);
  device.free(vertexBufferMemory);
  device.destroy(quadIndexBuffer);
  device.free(indexBufferMemory);
}

vk::CommandBuffer
BulkinBuffer::beginSingleTimeCommands(vk::Device device,
                                      vk::CommandPool &commandPool) {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;
  vk::CommandBuffer commandBuffer =
      std::move(device.allocateCommandBuffers(allocInfo).front());
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer.begin(beginInfo);
  return commandBuffer;
}

void BulkinBuffer::endSingleTimeCommands(vk::CommandBuffer commandBuffer,
                                         vk::Device device,
                                         vk::Queue graphicsQueue,
                                         vk::CommandPool commandPool) {
  commandBuffer.end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();

  device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}
