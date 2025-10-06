#include "quad.h"
#include <glm/gtc/matrix_transform.hpp>


const float shadingCoefs[6] = {
  1.0, 0.95, 0.9, 0.85, 0.8, 0.75
};

void BulkinQuad::addQuad(glm::vec3 position, float rotationX, float rotationY, float rotationZ, float scale, int shadingId, uint32_t textureIndex) {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
  model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
  model = glm::scale(model, glm::vec3(scale));
  matrices.push_back(model);
  shadingIds.push_back(shadingId);
  textureIndices.push_back(textureIndex);
}

uint32_t BulkinQuad::getInstanceCount() {
  return static_cast<uint32_t>(matrices.size());
}

PerInstanceData BulkinQuad::getInstanceData(size_t i) {
  return PerInstanceData {
    .model = matrices[i],
    .shadingId = shadingCoefs[shadingIds[i]],
    .textureIndex = textureIndices[i]
  };
}
