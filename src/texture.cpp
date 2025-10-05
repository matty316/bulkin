#include "texture.h"
#include "buffer.h"
#include "bulkin.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void BulkinTexture::load(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;
  
  if (!pixels)
    throw std::runtime_error("failed to load texture image");
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  
  BulkinBuffer::createBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  device.unmapMemory(stagingBufferMemory);
  
  stbi_image_free(pixels);
  
  Bulkin::createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, device, physicalDevice, image, imageMemory);
  Bulkin::transitionImageLayout(device, commandPool, graphicsQueue, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, image);
  copyBufferToImage(stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), device, commandPool, graphicsQueue);
  Bulkin::transitionImageLayout(device, commandPool, graphicsQueue, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, image);
  
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
  
  createImageView(device);
  createTextureSampler(device, physicalDevice);
}

void BulkinTexture::createImageView(vk::Device& device) {
  imageView = Bulkin::createImageView(device, image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void BulkinTexture::copyBufferToImage(vk::Buffer buffer, uint32_t width, uint32_t height, vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue) {
  auto commandBuffer = BulkinBuffer::beginSingleTimeCommands(device, commandPool);

  vk::BufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  
  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  
  region.imageOffset = vk::Offset3D{0, 0, 0};
  region.imageExtent = vk::Extent3D{width, height, 1};
  
  commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
  
  BulkinBuffer::endSingleTimeCommands(commandBuffer, device, graphicsQueue, commandPool);
}

void BulkinTexture::createTextureSampler(vk::Device& device, vk::PhysicalDevice& physicalDevice) {
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
  samplerInfo.anisotropyEnable = vk::True;
  vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = vk::False;
  samplerInfo.compareEnable = vk::False;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  sampler = device.createSampler(samplerInfo);
}

void BulkinTexture::cleanup(vk::Device &device) {
  device.destroy(image);
  device.free(imageMemory);
  device.destroy(imageView);
  device.destroy(sampler);
}
