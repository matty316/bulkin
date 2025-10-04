#include "bulkin.h"

#include <print>

void Bulkin::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void Bulkin::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(WIDTH, HEIGHT, "bulkin", fullsize ? glfwGetPrimaryMonitor() : nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Bulkin::framebufferResizeCallback(GLFWwindow *window, int width,
                                       int height) {
  auto app = reinterpret_cast<Bulkin *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void Bulkin::mouse_callback(GLFWwindow *window, double x, double y) {
  auto app = reinterpret_cast<Bulkin *>(glfwGetWindowUserPointer(window));
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  float xpos = static_cast<float>(x / width);
  float ypos = static_cast<float>(y / height);

  if (app->firstMouse) {
    app->lastX = xpos;
    app->lastY = ypos;
    app->firstMouse = false;
  }

  float xoffset = xpos - app->lastX;
  float yoffset = app->lastY - ypos;

  app->lastX = xpos;
  app->lastY = ypos;

  app->mouseState.pos.x = xoffset;
  app->mouseState.pos.y = yoffset;
}

void Bulkin::key_callback(GLFWwindow *window, int key, int scancode, int action,
                          int mods) {
  auto app = reinterpret_cast<Bulkin *>(glfwGetWindowUserPointer(window));
  const bool press = action != GLFW_RELEASE;
  if (key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_W)
    app->camera.movement.forward = press;
  if (key == GLFW_KEY_S)
    app->camera.movement.backward = press;
  if (key == GLFW_KEY_A)
    app->camera.movement.left = press;
  if (key == GLFW_KEY_D)
    app->camera.movement.right = press;
  if (key == GLFW_KEY_LEFT_SHIFT)
    app->camera.movement.fast = press;
}

void Bulkin::initVulkan() {
  createInstance();
  device.createSurface(instance, window);
  device.pickPhysicalDevice(instance);
  device.createLogicalDevice();
  device.createSwapchain(window);
  device.createGraphicsPipeline(quad, texture);
  createSyncObjects();
}

void Bulkin::mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    if (showFrametime)
      currentTime = glfwGetTime();
    glfwPollEvents();
    drawFrame();
    if (showFrametime)
      std::println("{} milliseconds", (glfwGetTime() - currentTime) * 1000);
  }

  device.device.waitIdle();
}

void Bulkin::drawFrame() {
  if (device.device.waitForFences(1, &drawFences[currentFrame], vk::True,
                                  UINT64_MAX) != vk::Result::eSuccess)
    throw std::runtime_error("failed to wait for fence");

  uint32_t imageIndex;
  auto result = device.device.acquireNextImageKHR(
      device.swapchain.swapchain, UINT64_MAX,
      presentCompleteSemaphores[currentFrame], nullptr, &imageIndex);

  if (result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapchain();
  } else if (result != vk::Result::eSuccess &&
             result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire next image");
  }

  if (device.device.resetFences(1, &drawFences[currentFrame]) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to reset fence");

  update();

  device.graphicsPipeline.commandBuffers[currentFrame].reset();
  device.graphicsPipeline.recordCommandBuffer(
      device.graphicsPipeline.commandBuffers[currentFrame], imageIndex,
      device.swapchain, currentFrame, quad);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {presentCompleteSemaphores[currentFrame]};
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &device.graphicsPipeline.commandBuffers[currentFrame];

  vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (device.graphicsQueue.submit(1, &submitInfo, drawFences[currentFrame]) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to submit graphics queue");

  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  vk::SwapchainKHR swapChains[] = {device.swapchain.swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = device.presentQueue.presentKHR(presentInfo);

  if (result == vk::Result::eErrorOutOfDateKHR ||
      result == vk::Result::eSuboptimalKHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapchain();
  } else if (result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to present graphics queue");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Bulkin::update() {
  const double newTimeStamp = glfwGetTime();
  deltaTime = newTimeStamp - timeStamp;
  timeStamp = newTimeStamp;
  camera.update(deltaTime, mouseState.pos);
  updatePushConstants();

  device.graphicsPipeline.buffers.updateUniformBuffer(
      currentFrame, static_cast<float>(device.swapchain.extent.width),
      static_cast<float>(device.swapchain.extent.height), camera);
}

void Bulkin::updatePushConstants() {
  //  glm::mat4 model(1.0f);
  //
  //  model = glm::translate(model, glm::vec3(0.0f));
  //  model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f));
  //  model = glm::scale(model, glm::vec3(1.0f));
  //
  //  pushConstants[currentFrame].model = model;
  //  pushConstants[currentFrame].view = camera.getView();
  //  pushConstants[currentFrame].proj = glm::perspective(glm::radians(45.0f),
  //  device.swapchain.extent.width / (float) device.swapchain.extent.height,
  //  0.1f, 100.0f);
}

void Bulkin::cleanup() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    device.device.destroy(presentCompleteSemaphores[i]);
    device.device.destroy(drawFences[i]);
  }
  for (size_t i = 0; i < device.swapchain.images.size(); i++) {
    device.device.destroy(renderFinishedSemaphores[i]);
  }
  texture.cleanup(device.device);
  device.cleanup(instance);
  instance.destroy();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Bulkin::createInstance() {
  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "bulkin";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "bulkin engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> requiredExtensions;

  for (uint32_t i = 0; i < glfwExtensionCount; i++)
    requiredExtensions.emplace_back(glfwExtensions[i]);

  requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);

  createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  createInfo.enabledLayerCount = 0;

  if (vk::createInstance(&createInfo, nullptr, &instance) !=
      vk::Result::eSuccess)
    throw std::runtime_error("failed to create instance");
}

void Bulkin::createSyncObjects() {
  size_t imageSize = device.swapchain.images.size();
  presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(imageSize);
  drawFences.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    presentCompleteSemaphores[i] =
        device.device.createSemaphore(vk::SemaphoreCreateInfo());
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    drawFences[i] = device.device.createFence(fenceInfo);
  }
  for (size_t i = 0; i < imageSize; i++) {
    renderFinishedSemaphores[i] =
        device.device.createSemaphore(vk::SemaphoreCreateInfo());
  }
}

void Bulkin::addQuad(glm::vec3 position, float rotationX, float rotationY,
                     float rotationZ, float scale, int shadingId) {
  quad.addQuad(position, rotationX, rotationY, rotationZ, scale, shadingId);
}

void Bulkin::setPlayerPos(glm::vec2 pos) { camera.setPlayerPos(pos); }

vk::ImageView Bulkin::createImageView(vk::Device& device, vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.image = image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  return device.createImageView(viewInfo);
}

void Bulkin::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Image& image, vk::DeviceMemory& imageMemory) {
  vk::ImageCreateInfo imageInfo{};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent.width = static_cast<uint32_t>(width);
  imageInfo.extent.height = static_cast<uint32_t>(height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = usage;
  imageInfo.sharingMode = vk::SharingMode::eExclusive;
  imageInfo.samples = vk::SampleCountFlagBits::e1;
  
  image = device.createImage(imageInfo);
  
  vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);
  
  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = BulkinBuffer::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);
 
  imageMemory = device.allocateMemory(allocInfo);
  device.bindImageMemory(image, imageMemory, 0);
}

void Bulkin::transitionImageLayout(vk::Device device, vk::CommandPool commandPool, vk::Queue graphicsQueue, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::Image& image) {
  auto commandBuffer = BulkinBuffer::beginSingleTimeCommands(device, commandPool);
  
  vk::ImageMemoryBarrier barrier{};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.image = image;
  if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    
    if (BulkinGraphicsPipeline::hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  
  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;
  
  if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    
    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    
    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    
    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
  } else {
    throw std::runtime_error("unsupported layout transition");
  }

  commandBuffer.pipelineBarrier(sourceStage,
                                destinationStage,
                                {},
                                0, nullptr,
                                0, nullptr,
                                1, &barrier);
  
  BulkinBuffer::endSingleTimeCommands(commandBuffer, device, graphicsQueue, commandPool);
}

void Bulkin::recreateSwapchain() {
  device.swapchain.recreate(device.device, device.surface, window, device.findQueueFamilies(device.physicalDevice));
  device.graphicsPipeline.createDepthResources(device.device, device.physicalDevice, device.graphicsQueue, device.swapchain.extent.width, device.swapchain.extent.height);
}
