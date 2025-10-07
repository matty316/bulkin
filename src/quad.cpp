#include "quad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

void BulkinQuad::addQuad(glm::vec3 position, float angle, glm::vec3 rotation, float scale, int faceId, uint32_t textureIndex) {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  glm::quat rot = glm::angleAxis(glm::radians(angle), rotation);
  model = model * glm::mat4_cast(rot);
  model = glm::scale(model, glm::vec3(scale));
  matrices.push_back(model);
  faceIds.push_back(faceId);
  textureIndices.push_back(textureIndex);
}

uint32_t BulkinQuad::getInstanceCount() {
  return static_cast<uint32_t>(matrices.size());
}

PerInstanceData BulkinQuad::getInstanceData(size_t i) {
  return PerInstanceData {
    .model = matrices[i],
    .faceId = faceIds[i],
    .textureIndex = textureIndices[i]
  };
}
