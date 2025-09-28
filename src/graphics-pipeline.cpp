#include "graphics-pipeline.h"
#include "constants.h"
#include "vertex.h"

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
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
    
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
  rasterizer.frontFace = vk::FrontFace::eClockwise;
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
  pipelineLayoutInfo.setLayoutCount = 0;
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

void BulkinGraphicsPipeline::recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, BulkinSwapchain& swapchain) {
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
  
  commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
  
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
}
