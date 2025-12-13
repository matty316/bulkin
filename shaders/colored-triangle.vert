#version 450

layout(location = 0) out vec3 outColor;

void main() {
  const vec3 pos[3] = vec3[3](
      vec3(1.0, 1.0, 0.0),
      vec3(-1.0, 1.0, 0.0),
      vec3(0.0, -1.0, 0.0)
    );

  const vec3 colors[3] = vec3[3](
      vec3(1.0, 0.0, 0.0),
      vec3(0.0, 1.0, 0.0),
      vec3(0.0, 0.0, 1.0)
    );

  gl_Position = vec4(pos[gl_VertexIndex], 1.0);
  outColor = colors[gl_VertexIndex];
}
