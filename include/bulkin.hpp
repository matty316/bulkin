#pragma once

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <vk_types.h>

#include "bulkin-framedata.hpp"
#include "bulkin-deletion.hpp"
#include "bulkin-image.hpp"
#include "bulkin-descriptor.hpp"
#include "bulkin-compute-effects.hpp"
#include "bulkin-buffer.hpp"
#include "bulkin-vertex.hpp"
#include "bulkin-loader.hpp"
#include "bulkin-scene.hpp"

class Bulkin {
public:
  static Bulkin &get();

  void init();
  void run();
  void cleanup();
  BulkinMeshBuffer upload_mesh(std::span<uint32_t> indices, std::span<BulkinVertex> vertices);
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
  VkExtent2D window_extent{1920, 1080};
  VkQueue graphics_queue;
  uint32_t graphics_queue_family;

  uint32_t frame_number = 0;

  struct SDL_Window *window;

  bool isInitialized = false;
  bool stop_rendering = false;
  bool resize_requested = false;

  BulkinFrameData frames[FRAME_OVERLAP];
  BulkinFrameData &get_current_frame() { return frames[frame_number % FRAME_OVERLAP]; }

  BulkinDeletionQueue deletion_queue;

  VmaAllocator allocator;

  BulkinImage draw_image;
  BulkinImage depth_image;
  VkExtent2D draw_extent;
  float render_scale = 1.0f;

  BulkinDescriptorAllocator descriptor_allocator;
  VkDescriptorSet draw_image_descriptors;
  VkDescriptorSetLayout draw_image_descriptor_layout;

  VkPipelineLayout gradient_pipeline_layout;

  VkFence imm_fence;
  VkCommandBuffer imm_cmd;
  VkCommandPool imm_cmd_pool;

  std::vector<BulkinComputeEffect> background_effects;
  int current_background_effect = 0;

  VkPipelineLayout mesh_pipeline_layout;
  VkPipeline mesh_pipeline;

  std::vector<std::shared_ptr<BulkinMeshAsset>> test_meshes;

  BulkinGPUSceneData scene_data;
  VkDescriptorSetLayout gpu_scene_data_descriptor_layout;

  BulkinImage white_image;
  BulkinImage black_image;
  BulkinImage grey_image;
  BulkinImage error_checkerboard_image;

  VkSampler default_sampler_linear;
  VkSampler default_sampler_nearest;

  VkDescriptorSetLayout single_image_descriptor_layout;

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_sync_structures();
  void create_swapchain(uint32_t width, uint32_t height);
  void destroy_swapchain();
  void init_descriptors();
  void init_pipelines();
  void init_background_pipelines();
  void init_imgui();
  void init_mesh_pipeline();
  void init_default_data();
  void imm_submit(std::function<void(VkCommandBuffer cmd)>&& function);
  void draw();
  void draw_background(VkCommandBuffer cmd);
  void draw_geometry(VkCommandBuffer cmd);
  void draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view);
  BulkinBuffer create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
  void destroy_buffer(const BulkinBuffer &buffer);
  void resize_swapchain();
  BulkinImage create_image(VkExtent3D size, VkFormat format, VkBufferUsageFlags usage, bool mipmapped = false);
  BulkinImage create_image(void *data, VkExtent3D size, VkFormat format, VkBufferUsageFlags usage, bool mipmapped = false);
  void destroy_image(const BulkinImage &img);
};
