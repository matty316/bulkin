#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include "device.h"
#include "constants.h"
#include "camera.h"

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
  bool framebufferResized = false;
 
  BulkinCamera camera = BulkinCamera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  
  struct MouseState {
    glm::vec2 pos = glm::vec2(0.0f);
    bool pressed = false;
  } mouseState;
  float lastX = WIDTH / 2.0f;
  float lastY = HEIGHT / 2.0f;
  bool firstMouse = true;
  
  double timeStamp = glfwGetTime();
  double deltaTime = 0.0f;
  
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
  void createInstance();
  void drawFrame();
  void update();
  void createSyncObjects();
  static void mouse_callback(GLFWwindow *window, double x, double y);
  static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
