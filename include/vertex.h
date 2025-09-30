#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  
  static vk::VertexInputBindingDescription bindingDesc() {
    vk::VertexInputBindingDescription bindingDescription{};
    
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;
    
    return bindingDescription;
  }
  
  static std::array<vk::VertexInputAttributeDescription, 2> attrDesc() {
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions;
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    
    return attributeDescriptions;
  }
};

const std::vector<Vertex> quadVertices = {
    {{-0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> quadIndices = {
    0, 1, 2, 2, 3, 0
};
