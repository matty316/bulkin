#include "bulkin-descriptor.hpp"

#include <vk_types.h>

void BulkinDescriptorLayout::add_binding(uint32_t binding, VkDescriptorType type) {
  VkDescriptorSetLayoutBinding newbind{};
  newbind.binding = binding;
  newbind.descriptorCount = 1;
  newbind.descriptorType = type;

  bindings.push_back(newbind);
}

void BulkinDescriptorLayout::clear() {
  bindings.clear();
}

VkDescriptorSetLayout BulkinDescriptorLayout::build(VkDevice device, VkShaderStageFlags shader_stages, void *pNext, VkDescriptorSetLayoutCreateFlags flags) {
  for (auto &b : bindings)
    b.stageFlags |= shader_stages;

  VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.pNext = pNext;

  info.pBindings = bindings.data();
  info.bindingCount = (uint32_t)bindings.size();
  info.flags = flags;

  VkDescriptorSetLayout set;
  VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

  return set;
}
