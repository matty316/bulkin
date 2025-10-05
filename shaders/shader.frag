#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in float shading;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

const vec3 gamma = vec3(2.2);
const vec3 fog_color = vec3(0.05);

void main() {
  vec3 color = texture(texSampler, fragTexCoord).rgb;

  color *= shading;
  //float fog_dist = gl_FragCoord.z / gl_FragCoord.w;
  //color = mix(color, fog_color, (1.0 - exp2(-0.015 * fog_dist * fog_dist)));

  color = pow(color, 1/gamma);


  outColor = vec4(color, 1.0);
}
