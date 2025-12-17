#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "bulkin-image.hpp"
#include "bulkin-descriptor.hpp"

class Bulkin;

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

struct GLTFMetallic_Roughness {
  BulkinMaterialPipeline opaque_pipeline;
  BulkinMaterialPipeline transparent_pipeline;
  VkDescriptorSetLayout material_layout;

  struct MaterialConstants {
    glm::vec4 color_factors;
    glm::vec4 metal_rough_factors;
    glm::vec4 padding[14];
  };

  struct MaterialResources {
    BulkinImage color_image;
    VkSampler color_sampler;
    BulkinImage metal_rough_image;
    VkSampler metal_rough_sampler;
    VkBuffer data_buffer;
    uint32_t data_buffer_offset;
  };

  BulkinDescriptorWriter writer;

  void build_pipelines(Bulkin *app);
  void clear(VkDevice device);

  BulkinMaterial write_material(VkDevice device, BulkinMaterialPass pass, const MaterialResources &resources, BulkinDescriptorAllocatorGrowable &descriptor_allocator);
};
