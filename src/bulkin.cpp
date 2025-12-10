#include "bulkin.hpp"

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_images.h>

#include <cassert>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <thread>
#include <chrono>
#include <vulkan/vk_enum_string_helper.h>

Bulkin *loadedEngine = nullptr;

#ifdef NDEBUG
constexpr bool useValidationLayers = false;
#else
constexpr bool useValidationLayers = true;
#endif

Bulkin &Bulkin::get() { return *loadedEngine; }

void Bulkin::init() {
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  window = SDL_CreateWindow("Bulkin'", window_extent.width, window_extent.height, window_flags);

  init_vulkan();
  init_swapchain();
  init_commands();
  init_sync_structures();
}

void Bulkin::init_vulkan() {
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Bulkin")
    .request_validation_layers(useValidationLayers)
    .use_default_debug_messenger()
    .require_api_version(1, 3, 0)
    .build();

  vkb::Instance vkb_inst = inst_ret.value();

  instance = vkb_inst.instance;
  debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

  VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
	  .set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder device_builder{ physical_device };
	vkb::Device vkbDevice = device_builder.build().value();

	device = vkbDevice.device;
	chosen_gpu = physical_device.physical_device;

	graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void Bulkin::create_swapchain(uint32_t width, uint32_t height) {
  vkb::SwapchainBuilder swapchain_builder{ chosen_gpu, device, surface };
  swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain = swapchain_builder
    .set_desired_format(VkSurfaceFormatKHR{ .format = swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_desired_extent(width, height)
    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    .build()
    .value();

  swapchain_extent = vkbSwapchain.extent;
  swapchain = vkbSwapchain.swapchain;
  swapchain_images = vkbSwapchain.get_images().value();
  swapchain_imageviews = vkbSwapchain.get_image_views().value();
}

void Bulkin::destroy_swapchain() {
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (auto &image_view : swapchain_imageviews)
    vkDestroyImageView(device, image_view, nullptr);
}

void Bulkin::init_swapchain() {
  create_swapchain(window_extent.width, window_extent.height);
}

void Bulkin::init_commands() {
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (size_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].command_pool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].command_pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].command_buffer));
  }
}

void Bulkin::init_sync_structures() {
  VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

  for (size_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &frames[i].render_fence));

    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frames[i].swapchain_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frames[i].render_semaphore));
  }
}

void Bulkin::run() {
  SDL_Event e;
  bool shouldQuit = false;

  while (!shouldQuit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_EVENT_QUIT) shouldQuit = true;

      if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
        stop_rendering = true;

      if (e.type == SDL_EVENT_WINDOW_RESTORED)
        stop_rendering = false;
    }

    if (stop_rendering) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
  }

  draw();
}

void Bulkin::draw() {
  int timeout = 1000000000;
  VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().render_fence, true, timeout));
  VK_CHECK(vkResetFences(device, 1, &get_current_frame().render_fence));

  uint32_t swapchain_image_index;
  VK_CHECK(vkAcquireNextImageKHR(device, swapchain, timeout, get_current_frame().swapchain_semaphore, nullptr, &swapchain_image_index));

  VkCommandBuffer cmd = get_current_frame().command_buffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));
  VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  vkutil::transition_image(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  VkClearColorValue clear_value;
  float flash = std::abs(std::sin(frame_number / 120.0f));
  clear_value = {{0.0f, 0.0f, flash, 1.0f}};

  VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
  vkCmdClearColorImage(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
  vkutil::transition_image(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame().swapchain_semaphore);
  VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().render_semaphore);

  VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

  VK_CHECK(vkQueueSubmit2(graphics_queue, 1 ,&submit, get_current_frame().render_fence));

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;
  present_info.pSwapchains = &swapchain;
  present_info.swapchainCount = 1;
  present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
  present_info.waitSemaphoreCount = 1;
  present_info.pImageIndices = &swapchain_image_index;

  VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));

  frame_number++;
}

void Bulkin::cleanup() {
  if (isInitialized) {
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
      vkDestroyCommandPool(device, frames[i].command_pool, nullptr);

      vkDestroyFence(device, frames[i].render_fence, nullptr);
      vkDestroySemaphore(device, frames[i].swapchain_semaphore, nullptr);
      vkDestroySemaphore(device, frames[i].render_semaphore, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);

    vkb::destroy_debug_utils_messenger(instance, debug_messenger);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);
  }
  loadedEngine = nullptr;
}
