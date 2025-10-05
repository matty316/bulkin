#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>

class BulkinTexture {
public:
  BulkinTexture(std::string filename) : filename(filename) {}
  
  vk::ImageView imageView;
  vk::Sampler sampler;
  
  void load(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue);
  void cleanup(vk::Device& device);

private:
  vk::Image image;
  vk::DeviceMemory imageMemory;
  std::string filename;
  static std::unordered_map<size_t, std::string> loadedTextures;
  static std::vector<BulkinTexture> textures;
  void createImageView(vk::Device& device);
  void createTextureSampler(vk::Device& device, vk::PhysicalDevice& physicalDevice);
  void copyBufferToImage(vk::Buffer buffer, uint32_t width, uint32_t height, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue);
};
