#include "bulkin-pipelines.hpp"

#include <vk_initializers.h>
#include <fstream>

bool vkutil::load_shader_module(const char *file_path, VkDevice device, VkShaderModule *out_module) {
  std::ifstream file(file_path, std::ios::ate | std::ios::binary);

  if (!file.is_open())
    return false;

  size_t file_size = (size_t)file.tellg();
  std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
  file.seekg(0);
  file.read((char*)buffer.data(), file_size);
  file.close();

  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.codeSize = buffer.size() * sizeof(uint32_t);
  create_info.pCode = buffer.data();

  VkShaderModule module;
  if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS) {
    return false;
  }

  *out_module = module;
  return true;
}
