#pragma once

#include <glm/glm.hpp>
#include <vector>

class Quad {
public:
  void addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale);
  uint32_t getInstanceCount();
  glm::mat4 getModelMatrix(size_t i);
private:
  std::vector<glm::mat4> matrices;
};
