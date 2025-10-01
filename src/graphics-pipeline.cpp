#include "graphics-pipeline.h"
#include "constants.h"
#include "vertex.h"
#include "frag-shader.h"
#include "vert-shader.h"

#include <fstream>

void BulkinGraphicsPipeline::create(vk::Device &device, vk::Format& swapchainFormat) {
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
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  
  pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
  
  vk::PipelineRenderingCreateInfo pipelineRenderingInfo{};
  pipelineRenderingInfo.colorAttachmentCount = 1;
  pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
  
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
  device.destroy(commandPool);
  device.destroy(pipelineLayout);
  device.destroy(pipeline);
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

void BulkinGraphicsPipeline::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, BulkinSwapchain& swapchain, uint32_t currentFrame) {
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
  
  vk::RenderingInfo renderingInfo{};
  renderingInfo.renderArea.offset = vk::Offset2D(0, 0);
  renderingInfo.renderArea.extent = swapchain.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &attachmentInfo;
  
  commandBuffer.beginRendering(renderingInfo);
  
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
  commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchain.extent.width), static_cast<float>(swapchain.extent.height), 0.0f, 1.0f));
  commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchain.extent));
  
  vk::Buffer vertexBuffers[] = {buffers.vertexBuffer};
  vk::DeviceSize offsets[] = {0};
  commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
  commandBuffer.bindIndexBuffer(buffers.indexBuffer, 0, vk::IndexType::eUint16);
  
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
  
  commandBuffer.drawIndexed(static_cast<uint32_t>(quadIndices.size()), 1, 0, 0, 0);
  
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

void BulkinGraphicsPipeline::createVertexBuffer(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Queue& graphicsQueue) {
  buffers.createVertexBuffer(device, physicalDevice, commandPool, graphicsQueue);
  buffers.createIndexBuffer(device, physicalDevice, commandPool, graphicsQueue);
  buffers.createUniformBuffers(device, physicalDevice);
  createDescriptorPool(device);
  createDescriptorSets(device);
}

void BulkinGraphicsPipeline::createDescriptorLayout(vk::Device& device) {
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
  
  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;
  
  descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
}

void BulkinGraphicsPipeline::createDescriptorPool(vk::Device &device) {
  vk::DescriptorPoolSize poolSize{};
  poolSize.type = vk::DescriptorType::eUniformBuffer;
  poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  
  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  
  descriptorPool = device.createDescriptorPool(poolInfo);
}

void BulkinGraphicsPipeline::createDescriptorSets(vk::Device &device) {
  std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();
 
  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  descriptorSets = device.allocateDescriptorSets(allocInfo);
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffers.uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
    
    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    
    device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
  }
}
