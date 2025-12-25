#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "bulkin-material.hpp"

struct BulkinGPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
  glm::vec4 ambient_color;
  glm::vec4 sunlight_dir;
  glm::vec4 sunlight_color;
};

struct BulkinRenderObject {
  uint32_t index_count;
  uint32_t first_index;
  VkBuffer index_buffer;
  BulkinMaterial *material;
  glm::mat4 transform;
  VkDeviceAddress vertex_buffer_address;
};
