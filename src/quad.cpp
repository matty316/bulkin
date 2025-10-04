#include "quad.h"
#include <glm/gtc/matrix_transform.hpp>


const float shadingCoefs[6] = {
  1.0, 0.95, 0.9, 0.85, 0.8, 0.75
};

void Quad::addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale, int shadingId) {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(scale));
  matrices.push_back(model);
  shadingIds.push_back(shadingId);
}

uint32_t Quad::getInstanceCount() {
  return static_cast<uint32_t>(matrices.size());
}

PerInstanceData Quad::getInstanceData(size_t i) {
  return PerInstanceData {
    .model = matrices[i],
    .shadingId = shadingCoefs[shadingIds[i]]
  };
}
