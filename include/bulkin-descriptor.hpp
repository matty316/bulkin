#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <span>

struct BulkinDescriptorLayout {
  std::vector<VkDescriptorSetLayoutBinding> bindings;

  void add_binding(uint32_t binding, VkDescriptorType type);
  void clear();
  VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shader_stages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct BulkinDescriptorAllocator {
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  VkDescriptorPool pool;

  void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
  void clear_descriptors(VkDevice device);
  void destroy_pool(VkDevice device);

  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

struct BulkinDescriptorAllocatorGrowable {
public:
  struct PoolSizeRatio {
    VkDescriptorType type;
    float ratio;
  };

  void init(VkDevice device, uint32_t initial_sets, std::span<PoolSizeRatio> pool_ratios);
  void clear_pools(VkDevice device);
  void destroy_pools(VkDevice device);

  VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext = nullptr);
private:
  VkDescriptorPool get_pool(VkDevice device);
  VkDescriptorPool create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);

  std::vector<PoolSizeRatio> ratios;
  std::vector<VkDescriptorPool> full_pools;
  std::vector<VkDescriptorPool> ready_pools;
  uint32_t sets_per_pool;
};
