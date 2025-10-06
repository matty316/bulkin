#pragma once

#include "bulkin.h"
#include "light.h"
#include <glm/glm.hpp>

#include <vector>

class BulkinLevel {
public:
  BulkinLevel(std::string path, uint32_t wallTexture, uint32_t floorTexture, uint32_t ceilingTexture, size_t maxHeight = 2);
  void renderLevel(Bulkin& app);
private:
  std::vector<std::vector<uint32_t>> walls;
  std::vector<std::vector<uint32_t>> floors;
  std::vector<std::vector<uint32_t>> ceilings;
  std::vector<PointLight> pointLights;
  size_t depth = 0, width = 0;
  glm::vec2 playerPos;
  uint32_t wallTexture = 0;
  uint32_t floorTexture = 0;
  uint32_t ceilingTexture = 0;
  size_t maxHeight = 2;
};
