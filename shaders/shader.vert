#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
} ubo;

struct PerInstanceData {
  mat4 model;
  float shadingId;
  uint textureId;
};

layout(set = 1, binding = 0, std430) readonly buffer SSBO {
  PerInstanceData data[];
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out float shading;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out uint fragTextureId;


void main() {
  //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
  mat4 model = data[gl_InstanceIndex].model; 
  gl_Position = ubo.proj * ubo.view * model * vec4(inPosition, 1.0);
  shading = data[gl_InstanceIndex].shadingId;
  fragColor = inColor;
  fragTextureId = data[gl_InstanceIndex].textureId;
  fragTexCoord = inTexCoord;
}

