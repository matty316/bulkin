#include "model.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void BulkinModel::loadModel() {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err, warning;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &err, modelPath.c_str())) {
      throw std::runtime_error(err);
  }
  
  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex{};
      
      vertex.pos = {
        attrib.vertices[3 * index.vertex_index + 0],
        attrib.vertices[3 * index.vertex_index + 1],
        attrib.vertices[3 * index.vertex_index + 2]
      };

      vertex.texCoord = {
        attrib.texcoords[2 * index.texcoord_index + 0],
        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
      };

      vertex.color = {1.0f, 1.0f, 1.0f};
      
      vertices.push_back(vertex);
      indices.push_back(static_cast<uint32_t>(indices.size()));
    }
  }
}

glm::mat4 BulkinModel::modelMatrix() {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, position);
  glm::quat rot = glm::angleAxis(glm::radians(angle), rotation);
  model = model * glm::mat4_cast(rot);
  model = glm::scale(model, glm::vec3(scale));
  return model;
}
