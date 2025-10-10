#pragma once

#include <vulkan/vulkan.hpp>

class BulkinTexture {
public:
  BulkinTexture(std::string filename) : filename(filename) {}
  
  vk::ImageView imageView;
  vk::Sampler sampler;
  
  void load(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void cleanup(vk::Device& device);

private:
  uint32_t mipLevels;
  vk::Image image;
  vk::DeviceMemory imageMemory;
  std::string filename;
  void createImageView(vk::Device& device);
  void createTextureSampler(vk::Device& device, vk::PhysicalDevice& physicalDevice);
  void copyBufferToImage(vk::Buffer buffer, uint32_t width, uint32_t height, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue);
  void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t, uint32_t mipLevels, vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
};
