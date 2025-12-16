#pragma once

#include <glm/glm.hpp>

struct BulkinGPUSceneData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
  glm::vec4 ambient_color;
  glm::vec4 sunlight_dir;
  glm::vec4 sunlight_color;
};
