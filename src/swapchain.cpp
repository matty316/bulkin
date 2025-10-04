#include "swapchain.h"
#include "bulkin.h"

void BulkinSwapchain::chooseSwapSurfaceFormat() {
  for (const auto& availableFormat : formats) {
    if (availableFormat.format == vk::Format::eB8G8R8Srgb) {
      format = availableFormat;
      return;
    }
  }
  format = formats[0];
}

void BulkinSwapchain::chooseSwapPresentMode() {
  for (const auto& availablePresentMode : presentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
      presentMode = availablePresentMode;
      return;
    }
  }
  presentMode = vk::PresentModeKHR::eFifo;
}

void BulkinSwapchain::chooseSwapExtent(GLFWwindow *window) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    extent = capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    vk::Extent2D actualExtext = {
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };
    
    actualExtext.width = std::clamp(actualExtext.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtext.height = std::clamp(actualExtext.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    
    extent = actualExtext;
  }
}

void BulkinSwapchain::querySwapchainSupport(vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface) {
  capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
  formats = physicalDevice.getSurfaceFormatsKHR(surface);
  presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
}

void BulkinSwapchain::createSwapchain(vk::Device& device, vk::SurfaceKHR& surface, GLFWwindow *window, QueueFamilyIndices indices) {
  chooseSwapSurfaceFormat();
  chooseSwapPresentMode();
  chooseSwapExtent(window);
  
  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    imageCount = capabilities.maxImageCount;
  
  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = format.format;
  createInfo.imageColorSpace = format.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
  
  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }
  
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = presentMode;
  createInfo.clipped = vk::True;
  createInfo.oldSwapchain = nullptr;
  
  swapchain = device.createSwapchainKHR(createInfo);
  
  images = device.getSwapchainImagesKHR(swapchain);
  imageFormat = format.format;
}

void BulkinSwapchain::createImageViews(vk::Device& device) {
  imageViews.resize(images.size());
  
  for (size_t i = 0; i < images.size(); i++) {
    imageViews[i] = Bulkin::createImageView(device, images[i], imageFormat, vk::ImageAspectFlagBits::eColor);
  }
}

void BulkinSwapchain::cleanup(vk::Device& device) {
  device.destroy(swapchain);
  for (const auto& imageView: imageViews)
    device.destroy(imageView);
}

bool BulkinSwapchain::isAdequate() {
  return !formats.empty() && !presentModes.empty();
}

void BulkinSwapchain::recreate(vk::Device& device, vk::SurfaceKHR& surface, GLFWwindow* window, QueueFamilyIndices indices) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  
  device.waitIdle();
  cleanup(device);
  createSwapchain(device, surface, window, indices);
  createImageViews(device);
}
