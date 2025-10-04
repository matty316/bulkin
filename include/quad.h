#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "vertex.h"

class Quad {
public:
  void addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale, int shadingId);
  uint32_t getInstanceCount();
  PerInstanceData getInstanceData(size_t i);
private:
  std::vector<glm::mat4> matrices;
  std::vector<int>shadingIds;
};
