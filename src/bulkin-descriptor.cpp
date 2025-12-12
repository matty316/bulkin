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

void BulkinDescriptorAllocator::init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios) {
  std::vector<VkDescriptorPoolSize> pool_sizes;
  for (auto &ratio : pool_ratios) {
    pool_sizes.push_back(VkDescriptorPoolSize{
      .type = ratio.type,
      .descriptorCount = static_cast<uint32_t>(ratio.ratio * max_sets)
    });
  }

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
  pool_info.pPoolSizes = pool_sizes.data();

  vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void BulkinDescriptorAllocator::clear_descriptors(VkDevice device) {

}

void BulkinDescriptorAllocator::destroy_pool(VkDevice device) {

}

VkDescriptorSet BulkinDescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {

}
