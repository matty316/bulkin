#include "texture.h"
#include "buffer.h"
#include "bulkin.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void BulkinTexture::load(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue) {
  int texWidth, texHeight, texChannels;
  stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  vk::DeviceSize imageSize = texWidth * texHeight * 4;
  
  mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
  
  if (!pixels)
    throw std::runtime_error("failed to load texture image");
  vk::Buffer stagingBuffer;
  vk::DeviceMemory stagingBufferMemory;
  
  BulkinBuffer::createBuffer(device, physicalDevice, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
  
  void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  device.unmapMemory(stagingBufferMemory);
  
  stbi_image_free(pixels);
  
  Bulkin::createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, device, physicalDevice, image, imageMemory, mipLevels);
  Bulkin::transitionImageLayout(device, commandPool, graphicsQueue, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, image, mipLevels);
  copyBufferToImage(stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), device, commandPool, graphicsQueue);
  generateMipmaps(image, vk::Format::eR8G8B8A8Srgb, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels, device, physicalDevice, commandPool, graphicsQueue);
  
  device.destroy(stagingBuffer);
  device.free(stagingBufferMemory);
  
  createImageView(device);
  createTextureSampler(device, physicalDevice);
}

void BulkinTexture::createImageView(vk::Device& device) {
  imageView = Bulkin::createImageView(device, image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
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
  samplerInfo.maxLod = vk::LodClampNone;
  sampler = device.createSampler(samplerInfo);
}

void BulkinTexture::cleanup(vk::Device &device) {
  device.destroy(image);
  device.free(imageMemory);
  device.destroy(imageView);
  device.destroy(sampler);
}

void BulkinTexture::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::CommandPool& commandPool, vk::Queue& graphicsQueue) {
  vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
  
  if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    throw std::runtime_error("texture image format does not support linear blitting");
  
  auto commandBuffer = BulkinBuffer::beginSingleTimeCommands(device, commandPool);
  
  vk::ImageMemoryBarrier barrier{};
  barrier.image = image;
  barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;
  
  int32_t mipWidth = texWidth;
  int32_t mipHeight = texHeight;
  
  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
    
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, nullptr, {}, nullptr, 1, &barrier);
    
    vk::ImageBlit blit{};
    blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
    blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
    blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    
    commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);
    
    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, nullptr, {}, nullptr, 1, &barrier);
    
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
  }
  
  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
  barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
  
  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, nullptr, {}, nullptr, 1, &barrier);
  
  BulkinBuffer::endSingleTimeCommands(commandBuffer, device, graphicsQueue, commandPool);
}
