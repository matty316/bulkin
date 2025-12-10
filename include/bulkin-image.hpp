#pragma once

#include <vulkan/vulkan.h>
#include <vk_types.h>

struct BulkinImage {
  VkImage image;
  VkImageView image_view;
  VmaAllocation allocation;
  VkExtent3D image_extent;
  VkFormat image_format;
};
