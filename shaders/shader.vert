#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
  mat4 view;
  mat4 proj;
  vec3 viewPos;
} ubo;

struct PerInstanceData {
  mat4 model;
  uint faceId;
  uint textureId;
};

layout(set = 1, binding = 0, std430) readonly buffer SSBO {
  PerInstanceData data[];
};

const vec3 normals[6] = vec3[6](
  vec3(0.0, 0.0, 1.0),  //front face
  vec3(0.0, 0.0, -1.0), //back face
  vec3(1.0, 0.0, 0.0),  //right face
  vec3(-1.0, 0.0, 0.0), //left face
  vec3(0.0, 1.0, 0.0),  //top face
  vec3(0.0, -1.0, 0.0)  //bottom face
);

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out uint fragTextureId;
layout(location = 4) out vec3 normal;
layout(location = 5) out vec3 viewPos;

void main() {
  //gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
  mat4 model = data[gl_InstanceIndex].model; 
  fragPos = vec3(model * vec4(inPosition, 1.0));
  fragColor = inColor;
  fragTextureId = data[gl_InstanceIndex].textureId;
  fragTexCoord = inTexCoord;
  normal = mat3(transpose(inverse(model))) * normals[data[gl_InstanceIndex].faceId]; 
  viewPos = ubo.viewPos;
  gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);
}

