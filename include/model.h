#pragma once

#include "vertex.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class BulkinModel {
public:
  BulkinModel(std::string modelPath, uint32_t texture, glm::vec3 pos, float angle, glm::vec3 rotation, float scale) : modelPath(modelPath), textureId(texture), pos(pos), angle(angle), rotation(rotation), scale(scale) {}
  void loadModel();
  glm::mat4 modelMatrix();
private:
  std::string modelPath;
  glm::vec3 pos;
  float angle;
  glm::vec3 rotation;
  float scale;
  uint32_t textureId;
  
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};
