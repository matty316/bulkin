#include "quad.h"
#include <glm/gtc/matrix_transform.hpp>

void Quad::addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale) {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(scale));
  matrices.push_back(model);
}

uint32_t Quad::getInstanceCount() {
  return static_cast<uint32_t>(matrices.size());
}

glm::mat4 Quad::getModelMatrix(size_t i) {
  return matrices[i];
}
