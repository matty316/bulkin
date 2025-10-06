#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "swapchain.h"
#include "queue-family.h"
#include "graphics-pipeline.h"

class BulkinDevice {
public:
  vk::PhysicalDevice physicalDevice = nullptr;
  vk::Device device;
  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::SurfaceKHR surface;
  BulkinSwapchain swapchain;
  BulkinGraphicsPipeline graphicsPipeline;

  void pickPhysicalDevice(vk::Instance& instance);
  void createLogicalDevice();
  void createSurface(vk::Instance& instance, GLFWwindow *window);
  void createSwapchain(GLFWwindow* window);
  void createGraphicsPipeline(BulkinQuad quad, std::vector<BulkinTexture>& textures, std::vector<PointLight>& pointLights);
  void cleanup(vk::Instance& instance);
  QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice& device);
private:
  bool isDeviceSuitable(vk::PhysicalDevice& physicalDevice);
  bool checkDeviceExtensionSupport(vk::PhysicalDevice& physicalDevice);
};
