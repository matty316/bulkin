#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct BulkinBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};
