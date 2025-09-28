#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include "device.h"
#include "constants.h"

class Bulkin {
public:
  void run();
  
private:
  GLFWwindow *window;
  vk::Instance instance;
  BulkinDevice device;
  std::vector<vk::Semaphore> presentCompleteSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence> drawFences;
  uint32_t currentFrame = 0;
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  bool framebufferResized = false;
  
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
  void createInstance();
  void drawFrame();
  void createSyncObjects();
};
