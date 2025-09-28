#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "queue-family.h"

class BulkinSwapchain {
public:
  vk::SwapchainKHR swapchain;
  std::vector<vk::Image> images;
  std::vector<vk::ImageView> imageViews;
  vk::Extent2D extent;
  vk::Format imageFormat;

  void nextImage();
  void querySwapchainSupport(vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface);
  void chooseSwapSurfaceFormat();
  void chooseSwapPresentMode();
  void chooseSwapExtent(GLFWwindow* window);
  void createSwapchain(vk::Device& device, vk::SurfaceKHR& surface, GLFWwindow *window, QueueFamilyIndices indices);
  void createImageViews(vk::Device& device);
  bool isAdequate();
  void cleanup(vk::Device& device);
private:
  vk::SurfaceFormatKHR format;
  vk::PresentModeKHR presentMode;
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> presentModes;
};
