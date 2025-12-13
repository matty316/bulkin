#pragma once

#include <vulkan/vulkan.h>

namespace vkutil {
  bool load_shader_module(const char* file_path, VkDevice device, VkShaderModule *out_module);
};
