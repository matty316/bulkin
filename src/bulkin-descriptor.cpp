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
  vkResetDescriptorPool(device, pool, 0);
}

void BulkinDescriptorAllocator::destroy_pool(VkDevice device) {
  vkDestroyDescriptorPool(device, pool, nullptr);
}

VkDescriptorSet BulkinDescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.pNext = nullptr;
  alloc_info.descriptorPool = pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  VkDescriptorSet ds;
  VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &ds));

  return ds;
}

VkDescriptorPool BulkinDescriptorAllocatorGrowable::get_pool(VkDevice device) {
  VkDescriptorPool new_pool;
  if (ready_pools.size() != 0) {
    new_pool = ready_pools.back();
    ready_pools.pop_back();
  } else {
    new_pool = create_pool(device, sets_per_pool, ratios);
    sets_per_pool = sets_per_pool * 1.5;
    if (sets_per_pool > 4092) {
      sets_per_pool = 4092;
    }
  }

  return new_pool;
}

VkDescriptorPool BulkinDescriptorAllocatorGrowable::create_pool(VkDevice device,  uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) {
  std::vector<VkDescriptorPoolSize> pool_sizes;
  for (PoolSizeRatio ratio : pool_ratios) {
    pool_sizes.push_back(VkDescriptorPoolSize {
      .type = ratio.type,
      .descriptorCount = uint32_t(ratio.ratio * set_count)
    });
  }

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = set_count;
  pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
  pool_info.pPoolSizes = pool_sizes.data();

  VkDescriptorPool new_pool;
  vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);
  return new_pool;
}

void BulkinDescriptorAllocatorGrowable::init(VkDevice device, uint32_t initial_sets, std::span<PoolSizeRatio> pool_ratios) {
  ratios.clear();

  for (auto r : pool_ratios) {
    ratios.push_back(r);
  }

  VkDescriptorPool new_pool = create_pool(device, initial_sets, pool_ratios);
  sets_per_pool = initial_sets * 1.5;
  ready_pools.push_back(new_pool);
}

void BulkinDescriptorAllocatorGrowable::clear_pools(VkDevice device) {
  for (auto p : ready_pools) {
    vkResetDescriptorPool(device, p, 0);
  }
  for (auto p : full_pools) {
    vkResetDescriptorPool(device, p, 0);
    ready_pools.push_back(p);
  }
  full_pools.clear();
}

void BulkinDescriptorAllocatorGrowable::destroy_pools(VkDevice device) {
  for (auto p : ready_pools) {
    vkDestroyDescriptorPool(device, p, nullptr);
  }
  ready_pools.clear();

  for (auto p : full_pools) {
    vkDestroyDescriptorPool(device, p, nullptr);
  }
  full_pools.clear();
}

VkDescriptorSet BulkinDescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext) {
  VkDescriptorPool pool_to_use = get_pool(device);

  VkDescriptorSetAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.pNext = pNext;
  alloc_info.descriptorPool = pool_to_use;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  VkDescriptorSet ds;
  VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &ds);

  if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
    full_pools.push_back(pool_to_use);
    pool_to_use = get_pool(device);
    alloc_info.descriptorPool = pool_to_use;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &ds));
  }

  ready_pools.push_back(pool_to_use);
  return ds;
}
