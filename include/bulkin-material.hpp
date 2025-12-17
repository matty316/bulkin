#pragma once

#include <vulkan/vulkan.h>

enum class BulkinMaterialPass: uint8_t {
  MainColor,
  Transparent,
  Other
};

struct BulkinMaterialPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct BulkinMaterial {
  BulkinMaterialPipeline *pipeline;
  VkDescriptorSet materialSet;
  BulkinMaterialPass passType;
};
