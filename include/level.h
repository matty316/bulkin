#pragma once

#include "bulkin.h"
#include "glm/glm.hpp"

#include <vector>

class BulkinLevel {
public:
  BulkinLevel(const char* path);
  void renderLevel(Bulkin& app);
private:
  std::vector<std::vector<uint32_t>> walls;
  size_t height = 0, width = 0;
  glm::vec2 playerPos;
};
