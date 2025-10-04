#pragma once

#include <vulkan/vulkan.hpp>

class BulkinTexture {
public:
  vk::ImageView imageView;
  vk::Sampler sampler;
  
  void load(const char* filename, vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void cleanup(vk::Device& device);

private:
  vk::Image image;
  vk::DeviceMemory imageMemory;
  
  void createImageView(vk::Device& device);
  void createTextureSampler(vk::Device& device, vk::PhysicalDevice& physicalDevice);
  void copyBufferToImage(vk::Buffer buffer, uint32_t width, uint32_t height, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue);
  
};
