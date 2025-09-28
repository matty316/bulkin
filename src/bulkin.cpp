#include "bulkin.h"

void Bulkin::run() {
  initWindow();
  initVulkan();
  mainLoop();
  cleanup();
}

void Bulkin::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(WIDTH, HEIGHT, "bulkin", nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void Bulkin::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto app = reinterpret_cast<Bulkin*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void Bulkin::initVulkan() {
  createInstance();
  device.createSurface(instance, window);
  device.pickPhysicalDevice(instance);
  device.createLogicalDevice();
  device.createSwapchain(window);
  device.createGraphicsPipeline();
  createSyncObjects();
}

void Bulkin::mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  
  device.device.waitIdle();
}

void Bulkin::drawFrame() {
  if(device.device.waitForFences(1, &drawFences[currentFrame], vk::True, UINT64_MAX) != vk::Result::eSuccess)
    throw std::runtime_error("failed to wait for fence");
  
  
  
  uint32_t imageIndex;
  auto result = device.device.acquireNextImageKHR(device.swapchain.swapchain,
                                                  UINT64_MAX,
                                                  presentCompleteSemaphores[currentFrame],
                                                  nullptr,
                                                  &imageIndex);
  
  if (result == vk::Result::eErrorOutOfDateKHR) {
    device.swapchain.recreate(device.device, device.surface, window, device.findQueueFamilies(device.physicalDevice));
  } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire next image");
  }
  
  if(device.device.resetFences(1, &drawFences[currentFrame]) != vk::Result::eSuccess)
    throw std::runtime_error("failed to reset fence");
  
  device.graphicsPipeline.commandBuffers[currentFrame].reset();
  device.graphicsPipeline.recordCommandBuffer(device.graphicsPipeline.commandBuffers[currentFrame], imageIndex, device.swapchain);
  
  vk::SubmitInfo submitInfo{};
  
  vk::Semaphore waitSemaphores[] = {presentCompleteSemaphores[currentFrame]};
  vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
  
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &device.graphicsPipeline.commandBuffers[currentFrame];
  
  vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  
  if (device.graphicsQueue.submit(1, &submitInfo, drawFences[currentFrame]) != vk::Result::eSuccess)
    throw std::runtime_error("failed to submit graphics queue");
  
  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  
  vk::SwapchainKHR swapChains[] = {device.swapchain.swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  
  result = device.presentQueue.presentKHR(presentInfo);
  
  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
    framebufferResized = false;
    device.swapchain.recreate(device.device, device.surface, window, device.findQueueFamilies(device.physicalDevice));
  } else if (result != vk::Result::eSuccess) {
    throw std::runtime_error("failed to present graphics queue");
  }
  
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Bulkin::cleanup() {
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    device.device.destroy(presentCompleteSemaphores[i]);
    device.device.destroy(drawFences[i]);
  }
  for (size_t i = 0; i < device.swapchain.images.size(); i++) {
    device.device.destroy(renderFinishedSemaphores[i]);
  }
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
  const char** glfwExtensions;
  
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  
  std::vector<const char*> requiredExtensions;
  
  for (uint32_t i = 0; i < glfwExtensionCount; i++)
    requiredExtensions.emplace_back(glfwExtensions[i]);
  
  requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
  
  createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
  
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();
  
  createInfo.enabledLayerCount = 0;
  
  if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess) 
    throw std::runtime_error("failed to create instance");
}

void Bulkin::createSyncObjects() {
  size_t imageSize = device.swapchain.images.size();
  presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(imageSize);
  drawFences.resize(MAX_FRAMES_IN_FLIGHT);
  
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    presentCompleteSemaphores[i] = device.device.createSemaphore(vk::SemaphoreCreateInfo());
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    drawFences[i] = device.device.createFence(fenceInfo);
  }
  for (size_t i = 0; i < imageSize; i++) {
    renderFinishedSemaphores[i] = device.device.createSemaphore(vk::SemaphoreCreateInfo());
  }
}
