#include "device.h"

#include <set>

std::vector<const char*> requiredExtensions{"VK_KHR_portability_subset", vk::KHRSwapchainExtensionName, "VK_KHR_shader_draw_parameters", vk::KHRSynchronization2ExtensionName};

QueueFamilyIndices BulkinDevice::findQueueFamilies(vk::PhysicalDevice& device) {
  QueueFamilyIndices indices;
  
  auto queueFamilies = device.getQueueFamilyProperties();
  
  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      indices.graphicsFamily = i;
    }
    
    auto presentSupport = device.getSurfaceSupportKHR(i, surface);
    
    if (presentSupport)
      indices.presentFamily = i;
    
    if (indices.isComplete())
      break;
    
    i++;
  }
  
  return indices;
}

void BulkinDevice::pickPhysicalDevice(vk::Instance& instance) {
  auto devices = instance.enumeratePhysicalDevices();
  if (devices.empty())
    throw std::runtime_error("failed to find GPUs with vulkan support");
    
  for (auto& device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }
  
  if (!physicalDevice)
    throw std::runtime_error("failed to find suitable GPU");
}

bool BulkinDevice::isDeviceSuitable(vk::PhysicalDevice& physicalDevice) {
  auto indices = findQueueFamilies(physicalDevice);
  
  bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);
  
  bool swapchainAdequate = false;
  if (extensionsSupported) {
    BulkinSwapchain swapchain;
    swapchain.querySwapchainSupport(physicalDevice, surface);
    swapchainAdequate = swapchain.isAdequate();
    if (swapchainAdequate)
      this->swapchain = swapchain;
  }
  return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

bool BulkinDevice::checkDeviceExtensionSupport(vk::PhysicalDevice& physicalDevice) {
  auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
  
  for (const auto& extension : requiredExtensions) {
    bool extensionFound = false;
    
    for (const auto& ext : availableExtensions) {
      if (strcmp(extension, ext.extensionName) == 0) {
        extensionFound = true;
        break;
      }
    }
    
    if (!extensionFound) {
      return false;
    }
  }
  
  return true;
}

void BulkinDevice::createLogicalDevice() {
  auto indices = findQueueFamilies(physicalDevice);
  
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
  
  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }
    
  vk::PhysicalDeviceFeatures2 deviceFeatures;
  vk::PhysicalDeviceVulkan13Features vulkan13Features;
  vulkan13Features.dynamicRendering = true;
  vulkan13Features.synchronization2 = true;
  vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extendedStateFeatures;
  extendedStateFeatures.extendedDynamicState = true;
  vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
      deviceFeatures,         // vk::PhysicalDeviceFeatures2 (empty for now)
      vulkan13Features,       // Enable dynamic rendering from Vulkan 1.3
      extendedStateFeatures   // Enable extended dynamic state from the extension
  };
  
  vk::DeviceCreateInfo createInfo{};
  createInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();
  createInfo.enabledLayerCount = 0;
  
  device = physicalDevice.createDevice(createInfo);
  
  device.getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
  device.getQueue(indices.presentFamily.value(), 0, &presentQueue);
}

void BulkinDevice::cleanup(vk::Instance& instance) {
  graphicsPipeline.cleanup(device);
  swapchain.cleanup(device);
  device.destroy();
  instance.destroy(surface);
}

void BulkinDevice::createSurface(vk::Instance &instance, GLFWwindow *window) {
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS)
    throw std::runtime_error("failed to create window surface");
  
  surface = vk::SurfaceKHR(_surface);
}

void BulkinDevice::createSwapchain(GLFWwindow *window) {
  swapchain.createSwapchain(device, surface, window, findQueueFamilies(physicalDevice));
  swapchain.createImageViews(device);
}

void BulkinDevice::createGraphicsPipeline() {
  graphicsPipeline.createDescriptorLayout(device);
  graphicsPipeline.create(device, swapchain.imageFormat);
  graphicsPipeline.createCommandPool(device, findQueueFamilies(physicalDevice));
  graphicsPipeline.createVertexBuffer(device, physicalDevice, graphicsQueue);
  graphicsPipeline.createCommandBuffers(device);
}
