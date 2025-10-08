#include "graphics-pipeline.h"
#include "constants.h"
#include "vertex.h"
#include "frag-shader.h"
#include "vert-shader.h"
#include "bulkin.h"

#include <fstream>

void BulkinGraphicsPipeline::create(vk::Device &device, vk::PhysicalDevice& physicalDevice, vk::Format& swapchainFormat) {
  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
  
  std::vector<vk::ShaderModule> modules;
  
  if (slang) {
    auto slangShaderCode = readFile("shaders/slang.spv");

    auto slangModule = createShaderModule(slangShaderCode, device);
    modules.push_back(slangModule);
    
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = slangModule;
    vertShaderStageInfo.pName = "vertMain";
    
    
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = slangModule;
    fragShaderStageInfo.pName = "fragMain";
  } else {
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    
    for (size_t i = 0; i < shaders_vert_spv_len; i++)
      vertShaderCode.push_back(shaders_vert_spv[i]);
    
    for (size_t i = 0; i < shaders_frag_spv_len; i++)
      fragShaderCode.push_back(shaders_frag_spv[i]);
    
    auto vertModule = createShaderModule(vertShaderCode, device);
    auto fragModule = createShaderModule(fragShaderCode, device);
    
    modules.push_back(vertModule);
    modules.push_back(fragModule);
    
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertModule;
    vertShaderStageInfo.pName = "main";
    
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragModule;
    fragShaderStageInfo.pName = "main";
  }
  
  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
  
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  
  auto bindingDesc = Vertex::bindingDesc();
  auto attrDesc = Vertex::attrDesc();
  
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();
  
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = vk::False;
  
  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  
  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = vk::False;
  rasterizer.rasterizerDiscardEnable = vk::False;
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = vk::False;
  
  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sampleShadingEnable = vk::False;
  multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
  
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = vk::False;
  
  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.logicOpEnable = vk::False;
  colorBlending.logicOp = vk::LogicOp::eCopy;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;
  
  std::vector<vk::DynamicState> dynamicStates = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };
  vk::PipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();
  
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  vk::DescriptorSetLayout descriptorSetLayouts[] = {descriptorSetLayout, ssboDescriptorSetLayout};
  pipelineLayoutInfo.setLayoutCount = 2;
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  
  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.depthTestEnable = vk::True;
  depthStencil.depthWriteEnable = vk::True;
  depthStencil.depthCompareOp = vk::CompareOp::eLess;
  depthStencil.depthBoundsTestEnable = vk::False;
  depthStencil.stencilTestEnable = vk::False;
  
  pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
  
  auto depthFormat = findDepthFormat(physicalDevice);
  vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
  pipelineRenderingInfo.colorAttachmentCount = 1;
  pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
  pipelineRenderingInfo.depthAttachmentFormat = depthFormat;
  
  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.pNext = &pipelineRenderingInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = nullptr;
  
  auto [result, graphicsPipeline] = device.createGraphicsPipeline(nullptr, pipelineInfo);
  if (result != vk::Result::eSuccess)
    throw std::runtime_error("cannot create graphics pipeline");
    
  pipeline = graphicsPipeline;
  
  for (auto shaderModule : modules)
    device.destroy(shaderModule);
}

std::vector<char> BulkinGraphicsPipeline::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  
  if (!file.is_open())
    throw std::runtime_error("failed to open file");
  
  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);
  
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  
  file.close();
  return buffer;
}

vk::ShaderModule BulkinGraphicsPipeline::createShaderModule(const std::vector<char> &code, vk::Device& device) {
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
  return device.createShaderModule(createInfo);
}

void BulkinGraphicsPipeline::cleanup(vk::Device &device) {
  buffers.cleanup(device);
  device.destroy(descriptorPool);
  device.destroy(descriptorSetLayout);
  device.destroy(ssboDescriptorSetLayout);
  device.destroy(ssboDescriptorPool);
  device.destroy(commandPool);
  device.destroy(pipelineLayout);
  device.destroy(pipeline);
  device.destroy(depthImage);
  device.destroy(depthImageView);
  device.free(depthImageMemory);
}

void BulkinGraphicsPipeline::createCommandPool(vk::Device &device, QueueFamilyIndices indices) {
  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
  
  commandPool = device.createCommandPool(poolInfo);
}

void BulkinGraphicsPipeline::createCommandBuffers(vk::Device &device) {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = commandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
  
  commandBuffers = device.allocateCommandBuffers(allocInfo);
}

void BulkinGraphicsPipeline::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, BulkinSwapchain& swapchain, uint32_t currentFrame, BulkinQuad quad, std::vector<BulkinModel>& models) {
  vk::CommandBufferBeginInfo beginInfo{};
  commandBuffer.begin(beginInfo);
  
  transitionImageLayout(imageIndex,
                        commandBuffer,
                        swapchain,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eColorAttachmentOptimal,
                        {},
                        vk::AccessFlagBits2::eColorAttachmentWrite,
                        vk::PipelineStageFlagBits2::eTopOfPipe,
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput);
  
  vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
  vk::RenderingAttachmentInfo attachmentInfo{};
  attachmentInfo.imageView = swapchain.imageViews[imageIndex];
  attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
  attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
  attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
  attachmentInfo.clearValue = clearColor;
  
  vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0.0f);
  vk::RenderingAttachmentInfo depthAttachmentInfo{};
  depthAttachmentInfo.imageView = depthImageView;
  depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachmentInfo.clearValue = clearDepth;
  
  vk::RenderingInfo renderingInfo{};
  renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
  renderingInfo.renderArea.extent = swapchain.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &attachmentInfo;
  renderingInfo.pDepthAttachment = &depthAttachmentInfo;
  
  commandBuffer.beginRendering(renderingInfo);
  
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
  commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchain.extent.width), static_cast<float>(swapchain.extent.height), 0.0f, 1.0f));
  commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain.extent));
  
  vk::Buffer vertexBuffers[] = {buffers.quadVertexBuffer};
  vk::DeviceSize offsets[] = {0};
  commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
  commandBuffer.bindIndexBuffer(buffers.quadIndexBuffer, 0, vk::IndexType::eUint32);
  
  vk::DescriptorSet descriptorSet[] = {descriptorSets[currentFrame], ssboDescriptorSets[currentFrame]};
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 2, descriptorSet, 0, nullptr);

  commandBuffer.drawIndexed(static_cast<uint32_t>(quadIndices.size()), quad.getInstanceCount(), 0, 0, 0);
  
  for (size_t i = 0; i < models.size(); i++) {
    vk::Buffer modelVertexBuffers[] = {buffers.modelVertexBuffers[i]};
    commandBuffer.bindVertexBuffers(0, 1, modelVertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(buffers.modelIndexBuffers[i], 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(models[i].getIndicesSize(), 1, 0, 0, quad.getInstanceCount());
  }
  
  commandBuffer.endRendering();
  
  transitionImageLayout(imageIndex,
                        commandBuffer,
                        swapchain,
                        vk::ImageLayout::eColorAttachmentOptimal,
                        vk::ImageLayout::ePresentSrcKHR,
                        vk::AccessFlagBits2::eColorAttachmentWrite,
                        {},
                        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                        vk::PipelineStageFlagBits2::eBottomOfPipe);
  
  commandBuffer.end();
}

void BulkinGraphicsPipeline::transitionImageLayout(uint32_t imageIndex,
                                                   vk::CommandBuffer commandBuffer,
                                                   BulkinSwapchain& swapchain,
                                                   vk::ImageLayout oldLayout,
                                                   vk::ImageLayout newLayout,
                                                   vk::AccessFlags2 srcAccessMask,
                                                   vk::AccessFlags2 dstAccessMask,
                                                   vk::PipelineStageFlags2 srcStageMask,
                                                   vk::PipelineStageFlags2 dstStageMask) {
  vk::ImageMemoryBarrier2 barrier{};
  barrier.srcStageMask = srcStageMask;
  barrier.srcAccessMask = srcAccessMask;
  barrier.dstStageMask = dstStageMask;
  barrier.dstAccessMask = dstAccessMask;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.image = swapchain.images[imageIndex];
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  
  vk::DependencyInfo dependencyInfo{};
  dependencyInfo.dependencyFlags = {};
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &barrier;
  
  commandBuffer.pipelineBarrier2(dependencyInfo);
}

void BulkinGraphicsPipeline::createBuffers(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue& graphicsQueue, BulkinQuad quad, std::vector<BulkinTexture>& textures, std::vector<PointLight>& pointLights, std::vector<BulkinModel>& models) {
  buffers.createBuffers(device, physicalDevice, commandPool, graphicsQueue, quad, pointLights, models);
  createDescriptorPool(device, static_cast<uint32_t>(textures.size()));
  createDescriptorSets(device, quad, textures, pointLights);
}

void BulkinGraphicsPipeline::createDescriptorLayout(vk::Device& device, uint32_t textureCount) {
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  
  vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = textureCount;
  samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  
  vk::DescriptorSetLayoutBinding pointLightBinding{};
  pointLightBinding.binding = 2;
  pointLightBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  pointLightBinding.descriptorCount = 1;
  pointLightBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  
  std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, samplerLayoutBinding, pointLightBinding};
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  
  descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
  
  vk::DescriptorSetLayoutBinding ssboLayoutBinding{};
  ssboLayoutBinding.binding = 0;
  ssboLayoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
  ssboLayoutBinding.descriptorCount = 1;
  ssboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  
  vk::DescriptorSetLayoutCreateInfo ssboLayoutInfo{};
  ssboLayoutInfo.bindingCount = 1;
  ssboLayoutInfo.pBindings = &ssboLayoutBinding;
  
  ssboDescriptorSetLayout = device.createDescriptorSetLayout(ssboLayoutInfo);
}

void BulkinGraphicsPipeline::createDescriptorPool(vk::Device &device, uint32_t textureCount) {
  std::array<vk::DescriptorPoolSize, 3> poolSizes;
  poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
  poolSizes[1].descriptorCount = textureCount * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[2].type = vk::DescriptorType::eStorageBuffer;
  poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  
  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  
  descriptorPool = device.createDescriptorPool(poolInfo);
  
  vk::DescriptorPoolSize ssboPoolSize;
  ssboPoolSize.type = vk::DescriptorType::eStorageBuffer;
  ssboPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
  vk::DescriptorPoolCreateInfo ssboPoolInfo{};
  ssboPoolInfo.poolSizeCount = 1;
  ssboPoolInfo.pPoolSizes = &ssboPoolSize;
  ssboPoolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  
  ssboDescriptorPool = device.createDescriptorPool(ssboPoolInfo);
}

void BulkinGraphicsPipeline::createDescriptorSets(vk::Device &device, BulkinQuad quad, std::vector<BulkinTexture>& textures, std::vector<PointLight>& pointLights) {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();
 
  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  descriptorSets = device.allocateDescriptorSets(allocInfo);
  
  std::vector<vk::DescriptorSetLayout> ssboLayouts(MAX_FRAMES_IN_FLIGHT, ssboDescriptorSetLayout);
  vk::DescriptorSetAllocateInfo ssboAllocInfo{};
  ssboAllocInfo.descriptorPool = ssboDescriptorPool;
  ssboAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  ssboAllocInfo.pSetLayouts = ssboLayouts.data();
 
  ssboDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  ssboDescriptorSets = device.allocateDescriptorSets(ssboAllocInfo);
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = buffers.uniformBuffers[i];
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformBufferObject);
    
    std::vector<vk::DescriptorImageInfo> imageInfos{};
    imageInfos.resize(textures.size());
    for (size_t i = 0; i < textures.size(); i++) {
      vk::DescriptorImageInfo imageInfo{};
      imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      imageInfo.imageView = textures[i].imageView;
      imageInfo.sampler = textures[i].sampler;
      imageInfos[i] = imageInfo;
    }
    
    vk::DescriptorBufferInfo pointLightBufferInfo{};
    pointLightBufferInfo.buffer = buffers.pointLightBuffer;
    pointLightBufferInfo.offset = 0;
    pointLightBufferInfo.range = sizeof(pointLights[0]) * pointLights.size();
    
    vk::DescriptorBufferInfo ssboBufferInfo{};
    ssboBufferInfo.buffer = buffers.ssboBuffer;
    ssboBufferInfo.offset = 0;
    ssboBufferInfo.range = sizeof(PerInstanceData) * quad.getInstanceCount();
    
    std::array<vk::WriteDescriptorSet, 4> descriptorWrites{};
    
    descriptorWrites[0].dstSet = descriptorSets[i];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
    
    descriptorWrites[1].dstSet = descriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrites[1].descriptorCount = static_cast<uint32_t>(imageInfos.size());
    descriptorWrites[1].pImageInfo = imageInfos.data();
    
    descriptorWrites[2].dstSet = descriptorSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = vk::DescriptorType::eStorageBuffer;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &pointLightBufferInfo;
    
    descriptorWrites[3].dstSet = ssboDescriptorSets[i];
    descriptorWrites[3].dstBinding = 0;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = vk::DescriptorType::eStorageBuffer;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &ssboBufferInfo;
    
    device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
}

void BulkinGraphicsPipeline::createDepthResources(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue graphicsQueue, uint32_t width, uint32_t height) {
  auto depthFormat = findDepthFormat(physicalDevice);
  
  Bulkin::createImage(width,
                      height,
                      depthFormat,
                      vk::ImageTiling::eOptimal,
                      vk::ImageUsageFlagBits::eDepthStencilAttachment,
                      vk::MemoryPropertyFlagBits::eDeviceLocal,
                      device,
                      physicalDevice,
                      depthImage,
                      depthImageMemory);
  depthImageView = Bulkin::createImageView(device, depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
  Bulkin::transitionImageLayout(device, commandPool, graphicsQueue, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, depthImage);
}

vk::Format BulkinGraphicsPipeline::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice& physicalDevice) {
  for (vk::Format format : candidates) {
    auto props = physicalDevice.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  
  throw std::runtime_error("failed to find supported format");
}

vk::Format BulkinGraphicsPipeline::findDepthFormat(vk::PhysicalDevice& physicalDevice) {
  return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, physicalDevice);
}

bool BulkinGraphicsPipeline::hasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}
