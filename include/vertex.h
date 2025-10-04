#pragma once

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct UniformBufferObject {
  glm::mat4 view;
  glm::mat4 proj;
};

struct PerInstanceData {
  glm::mat4 model;
  float shadingId;
  glm::vec3 padding;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec2 texCoord;
  glm::vec3 color = {1.f, 1.f, 1.f};
  
  static vk::VertexInputBindingDescription bindingDesc() {
    vk::VertexInputBindingDescription bindingDescription{};
    
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    
    return bindingDescription;
  }
  
  static std::array<vk::VertexInputAttributeDescription, 3> attrDesc() {
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[2].offset = offsetof(Vertex, color);
    
    return attributeDescriptions;
  }
};

const std::vector<Vertex> quadVertices = {
  {{-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}},
  {{0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}},
  {{0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}},
  {{-0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> quadIndices = {
    0, 1, 2, 2, 3, 0
};
