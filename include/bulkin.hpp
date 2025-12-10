#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

#include "bulkin-framedata.hpp"

class Bulkin {
public:
  static Bulkin &get();

  void init();
  void run();
  void cleanup();
private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice chosen_gpu;
  VkDevice device;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkFormat swapchain_image_format;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_imageviews;
  VkExtent2D swapchain_extent;
  VkExtent2D window_extent{1700, 900};
  VkQueue graphics_queue;
  uint32_t graphics_queue_family;

  uint32_t frame_number = 0;

  struct SDL_Window *window;

  bool isInitialized = false;
  bool stop_rendering = false;

  FrameData frames[FRAME_OVERLAP];
  FrameData &get_current_frame() { return frames[frame_number % FRAME_OVERLAP]; }

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
  void draw();
};
