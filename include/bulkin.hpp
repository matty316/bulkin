#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <vk_types.h>

#include "bulkin-framedata.hpp"
#include "bulkin-deletion.hpp"
#include "bulkin-image.hpp"

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

  BulkinFrameData frames[FRAME_OVERLAP];
  BulkinFrameData &get_current_frame() { return frames[frame_number % FRAME_OVERLAP]; }

  BulkinDeletionQueue deletion_queue;

  VmaAllocator allocator;

  BulkinImage draw_image;
  VkExtent2D draw_extent;

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
  void draw();
};
