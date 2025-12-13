#pragma once

#include <vulkan/vulkan.h>
#include "bulkin-push-constants.hpp"

struct BulkinComputeEffect {
  const char* name;
  VkPipeline pipeline;
  VkPipelineLayout layout;
  BulkinPushConstants data;
};
