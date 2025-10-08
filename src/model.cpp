#include "model.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <glm/gtc/quaternion.hpp>

#include <unordered_map>

void BulkinModel::loadModel() {
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(modelPath.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_FlipUVs |
                                             aiProcess_GenSmoothNormals |
                                             aiProcess_SplitLargeMeshes |
                                             aiProcess_ImproveCacheLocality |
                                             aiProcess_RemoveRedundantMaterials |
                                             aiProcess_FindDegenerates |
                                             aiProcess_FindInvalidData |
                                             aiProcess_GenUVCoords |
                                             aiProcess_CalcTangentSpace);
    
    if (scene == nullptr || !scene->HasMeshes())
      throw std::runtime_error("unable to load model");
    
    for (size_t i = 0; i < scene->mNumMeshes; i++) {
      auto mesh = scene->mMeshes[i];
      for (size_t j = 0; j < mesh->mNumVertices; j++) {
        Vertex vertex;
        vertex.pos = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};
        vertex.texCoord = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
        vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[i].y, mesh->mNormals[j].z};
        vertex.color = {1.0f, 1.0f, 1.0f};
        vertices.push_back(vertex);
      }
      for (size_t j = 0; j < mesh->mNumFaces; j++) {
        aiFace face = mesh->mFaces[j];
        for (size_t k = 0; k < face.mNumIndices; k++) {
          indices.push_back(face.mIndices[k]);
        }
      }
      if (mesh->mMaterialIndex >= 0) {
        auto* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString str;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        auto directory = modelPath.substr(0, modelPath.find_last_of('/'));
        diffusePath = directory + "/" + str.C_Str();
      }
    }
}

glm::mat4 BulkinModel::modelMatrix() {
  auto model = glm::mat4(1.0f);
  model = glm::translate(model, pos);
  glm::quat rot = glm::angleAxis(glm::radians(angle), rotation);
  model = model * glm::mat4_cast(rot);
  model = glm::scale(model, glm::vec3(scale));
  return model;
}

std::vector<uint32_t> BulkinModel::getIndices() {
  return indices;
}

std::vector<Vertex> BulkinModel::getVertices() {
  return vertices;
}

uint32_t BulkinModel::getIndicesSize() {
  return static_cast<uint32_t>(indices.size());
}

uint32_t BulkinModel::getVerticesSize() {
  return static_cast<uint32_t>(vertices.size());
}
