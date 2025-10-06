#version 460

#extension GL_EXT_nonuniform_qualifier : require

struct PointLight {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float constant;
  float linear;
  float quadratic;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) flat in uint fragTextureId;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec3 viewPos;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSamplers[];

layout(std430, set = 0, binding = 2) readonly buffer PointLights { 
  PointLight pointLights[];
};

const vec3 gamma = vec3(2.2);
const vec3 fog_color = vec3(0.05);

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
  vec3 norm = normalize(normal);
  vec3 viewDir = normalize(viewPos - fragPos);

  vec3 result = vec3(0.0);

  for (int i = 0; i < pointLights.length(); i++)
    result += CalcPointLight(pointLights[i], norm, fragPos, viewDir);

  result = pow(result, 1/gamma);
  outColor = vec4(result, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);

        // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(texSamplers[fragTextureId], fragTexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texSamplers[fragTextureId], fragTexCoord));
    vec3 specular = light.specular * spec * vec3(texture(texSamplers[fragTextureId], fragTexCoord));
    ambient  *= attenuation;
    diffuse  *= attenuation;
   specular *= attenuation;
    return (ambient + diffuse + specular);
}
