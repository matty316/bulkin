#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "bulkin-buffer.hpp"

struct BulkinVertex {
  glm::vec3 position;
  float uv_x;
  glm::vec3 normal;
  float uv_y;
  glm::vec4 color;
};

struct BulkinMeshBuffer {
  BulkinBuffer index_buffer;
  BulkinBuffer vertex_buffer;
  VkDeviceAddress vertex_buffer_address;
};

struct BulkinDrawPushConstants {
  glm::mat4 world_matrix;
  VkDeviceAddress vertex_buffer;
};
