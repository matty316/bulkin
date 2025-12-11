#pragma once

#include <vector>
#include <vulkan/vulkan.h>

struct BulkinDescriptorLayout {
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  void add_binding(uint32_t binding, VkDescriptorType type);
  void clear();
  VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shader_stages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};
