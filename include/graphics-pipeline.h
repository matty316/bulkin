#pragma once

#include "queue-family.h"
#include "swapchain.h"
#include "buffer.h"
#include "quad.h"
#include "vertex.h"
#include "texture.h"

#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class BulkinGraphicsPipeline {
public:
  BulkinBuffer buffers;
  vk::CommandPool commandPool;
  std::vector<vk::CommandBuffer> commandBuffers;
  
  void create(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Format& swapchainFormat);
  void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, BulkinSwapchain& swapchain, uint32_t currentFrame, Quad quad);
  void createCommandPool(vk::Device& device, QueueFamilyIndices indices);
  void createCommandBuffers(vk::Device& device);
  void createBuffers(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue& graphicsQueue, Quad quad, BulkinTexture& texture);
  void createDescriptorLayout(vk::Device& device);
  void createDescriptorPool(vk::Device& device);
  void createDescriptorSets(vk::Device& device, Quad quad, BulkinTexture& texture);
  static bool hasStencilComponent(vk::Format format);
  void createDepthResources(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue graphicsQueue, uint32_t width, uint32_t height);
  void cleanup(vk::Device& device);
private:
  vk::PipelineLayout pipelineLayout;
  vk::Pipeline pipeline;
  vk::DescriptorSetLayout descriptorSetLayout;
  vk::DescriptorSetLayout ssboDescriptorSetLayout;
  vk::DescriptorPool descriptorPool;
  vk::DescriptorPool ssboDescriptorPool;
  std::vector<vk::DescriptorSet> descriptorSets;
  std::vector<vk::DescriptorSet> ssboDescriptorSets;
  bool slang = false;
  
  vk::Image depthImage;
  vk::DeviceMemory depthImageMemory;
  vk::ImageView depthImageView;
  
  vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice& physicalDevice);
  vk::Format findDepthFormat(vk::PhysicalDevice& physicalDevice);
  
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
