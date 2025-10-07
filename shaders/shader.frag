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

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightColor);

void main() {
  vec3 norm = normalize(normal);

  vec3 color = texture(texSamplers[fragTextureId], fragTexCoord).rgb;
  vec3 lighting = vec3(0.0);

  for (int i = 0; i < pointLights.length(); i++)
    lighting += BlinnPhong(norm, fragPos, pointLights[i].position, pointLights[i].diffuse);
  color *= lighting;

  color = pow(color, 1/gamma);
  outColor = vec4(color, 1.0);
}

vec3 BlinnPhong(vec3 normal, vec3 fragPos, vec3 lightPos, vec3 lightColor)
{
    vec3 ambient = lightColor * 0.1;
    // diffuse
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;    
    // simple attenuation
    float max_distance = 1.5;
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / distance;
    
    diffuse *= attenuation;
    specular *= attenuation;
    ambient *= attenuation;
    
    return diffuse;
}
