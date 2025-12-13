#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class BulkinPipeline {
public:
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

};

namespace vkutil {
  bool load_shader_module(const char* file_path, VkDevice device, VkShaderModule *out_module);
};
