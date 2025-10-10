#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <print>

#include "camera.h"
#include "constants.h"
#include "device.h"
#include "light.h"
#include "quad.h"
#include "vertex.h"
#include "model.h"

class Bulkin {
public:
  void run();
  void addQuad(glm::vec3 position, float angle, glm::vec3 rotation, float scale,
               int shadingId, uint32_t textureId);
  void addPointLight(PointLight &light);
  void setPlayerPos(glm::vec2 pos);
  uint32_t addTexture(std::string filename);
  void addModel(std::string modelPath, glm::vec3 pos, float angle, glm::vec3 rotation, float scale);

  static vk::ImageView createImageView(vk::Device &device,
                                       vk::Image image,
                                       vk::Format format,
                                       vk::ImageAspectFlags aspectFlags,
                                       uint32_t mipLevels);
  static void createImage(uint32_t width,
                          uint32_t height,
                          vk::Format format,
                          vk::ImageTiling tiling,
                          vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties,
                          vk::Device &device,
                          vk::PhysicalDevice &physicalDevice,
                          vk::Image &image,
                          vk::DeviceMemory &imageMemory,
                          uint32_t mipLevels);
  static void transitionImageLayout(vk::Device device,
                                    vk::CommandPool commandPool,
                                    vk::Queue graphicsQueue,
                                    vk::Format format,
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout,
                                    vk::Image &image,
                                    uint32_t mipLevels);

private:
  GLFWwindow *window;
  vk::Instance instance;
  BulkinDevice device;
  std::vector<vk::Semaphore> presentCompleteSemaphores;
  std::vector<vk::Semaphore> renderFinishedSemaphores;
  std::vector<vk::Fence> drawFences;
  uint32_t currentFrame = 0;
  bool framebufferResized = false;
  bool fullsize = false;
  BulkinQuad quad;
  std::vector<BulkinModel> models;

  bool showFrametime = false;
  double currentTime = 0.0;

  std::unordered_map<std::string, uint32_t> loadedTextures;
  std::vector<BulkinTexture> textures;

  std::vector<PointLight> pointLights;

  BulkinCamera camera =
      BulkinCamera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                   glm::vec3(0.0f, 1.0f, 0.0f));

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
  void updatePushConstants();
  void recreateSwapchain();
  static void mouse_callback(GLFWwindow *window, double x, double y);
  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods);
  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods);
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);

  bool tick(float deltaTime, bool frameRendered = true);
  
  float avgInterval = 0.5f;
  uint32_t numFrames = 0;
  double accumTime = 0.0;
  float currentFPS = 0.0f;
  bool printFPS = true;
};
