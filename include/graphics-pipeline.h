#pragma once

#include "queue-family.h"
#include "swapchain.h"
#include "buffer.h"

#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class BulkinGraphicsPipeline {
public:
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline pipeline;
  vk::CommandPool commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  bool slang = false;
  
  void create(vk::Device& device, vk::Format& swapchainFormat);
  void createCommandPool(vk::Device& device, QueueFamilyIndices indices);
  void createCommandBuffers(vk::Device& device);
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, BulkinSwapchain& swapchain);
  void createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue& graphicsQueue);
  void cleanup(vk::Device& device);
private:
  BulkinBuffer buffers;
  static std::vector<char> readFile(const std::string& filename);
  vk::ShaderModule createShaderModule(const std::vector<char>& code, vk::Device &device);
  void transitionImageLayout(uint32_t imageIndex,
                             vk::CommandBuffer commandBuffer,
                             BulkinSwapchain& swapchain,
                             vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout,
                             vk::AccessFlags2 srcAccessMask,
                             vk::AccessFlags2 dstAccessMask,
                             vk::PipelineStageFlags2 srcStageMask,
                             vk::PipelineStageFlags2 dstStageMask);
};
