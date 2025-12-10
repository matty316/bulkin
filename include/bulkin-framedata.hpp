#pragma once

#include <vulkan/vulkan.h>

struct FrameData {
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
  VkSemaphore swapchain_semaphore, render_semaphore;
  VkFence render_fence;
};

constexpr uint32_t FRAME_OVERLAP = 2;
