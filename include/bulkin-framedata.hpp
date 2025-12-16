#pragma once

#include <vulkan/vulkan.h>
#include "bulkin-deletion.hpp"
#include "bulkin-descriptor.hpp"

struct BulkinFrameData {
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
  VkSemaphore swapchain_semaphore, render_semaphore;
  VkFence render_fence;
  BulkinDeletionQueue deletion_queue;
  BulkinDescriptorAllocatorGrowable frame_descriptors;
};

constexpr uint32_t FRAME_OVERLAP = 2;
