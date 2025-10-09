#pragma once

#include "vertex.h"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class BulkinModel {
public:
  BulkinModel(std::string modelPath, glm::vec3 pos, float angle, glm::vec3 rotation, float scale) : modelPath(modelPath), pos(pos), angle(angle), rotation(rotation), scale(scale) {}
  void loadModel();
  glm::mat4 modelMatrix();
  std::vector<uint32_t> getIndices();
  uint32_t getIndicesSize();
  std::vector<Vertex> getVertices();
  uint32_t getVerticesSize();
  void setDiffuse(uint32_t diffuse) { textureId = diffuse; }
  uint32_t getTextureId() { return textureId; }
  std::string getDiffusePath() { return diffusePath; }

private:
  std::string modelPath;
  std::string diffusePath;
  uint32_t textureId;
  glm::vec3 pos;
  float angle;
  glm::vec3 rotation;
  float scale;
  
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};
