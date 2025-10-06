#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "vertex.h"

class BulkinQuad {
public:
  void addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale, int faceId, uint32_t textureIndex);
  uint32_t getInstanceCount();
  PerInstanceData getInstanceData(size_t i);
private:
  std::vector<glm::mat4> matrices;
  std::vector<uint32_t> faceIds;
  std::vector<uint32_t> textureIndices;
};
