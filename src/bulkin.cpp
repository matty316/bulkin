#include "bulkin.hpp"

#include <vk_types.h>
#include <vk_initializers.h>
#include <vk_images.h>

#include <cassert>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <thread>
#include <chrono>
#include <vulkan/vk_enum_string_helper.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "bulkin-pipelines.hpp"
#include "bulkin-push-constants.hpp"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

Bulkin *loadedEngine = nullptr;

#ifdef NDEBUG
constexpr bool useValidationLayers = false;
#else
constexpr bool useValidationLayers = true;
#endif

Bulkin &Bulkin::get() { return *loadedEngine; }

void Bulkin::init() {
  assert(loadedEngine == nullptr);
  loadedEngine = this;

  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  window = SDL_CreateWindow("Bulkin'", window_extent.width, window_extent.height, window_flags);

  init_vulkan();
  init_swapchain();
  init_commands();
  init_sync_structures();
  init_descriptors();
  init_pipelines();
  init_imgui();
  init_default_data();

  isInitialized = true;
}

void Bulkin::init_vulkan() {
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Bulkin")
    .request_validation_layers(useValidationLayers)
    .set_debug_callback (
        [] (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageType,
	    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	    void *pUserData)
            -> VkBool32 {
			auto severity = vkb::to_string_message_severity(messageSeverity);
			auto type = vkb::to_string_message_type(messageType);
			printf ("[%s: %s] %s\n", severity, type, pCallbackData->pMessage);
			return VK_FALSE;
		}
    )
    .require_api_version(1, 3, 0)
    .build();

  vkb::Instance vkb_inst = inst_ret.value();

  instance = vkb_inst.instance;
  debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

  VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physical_device = selector
	  .set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder device_builder{ physical_device };
	vkb::Device vkbDevice = device_builder.build().value();

	device = vkbDevice.device;
	chosen_gpu = physical_device.physical_device;

	graphics_queue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphics_queue_family = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocator_info{};
	allocator_info.physicalDevice = chosen_gpu;
	allocator_info.device = device;
	allocator_info.instance = instance;
	allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	vmaCreateAllocator(&allocator_info, &allocator);

	deletion_queue.push_function([&](){
    vmaDestroyAllocator(allocator);
	});
}

void Bulkin::create_swapchain(uint32_t width, uint32_t height) {
  vkb::SwapchainBuilder swapchain_builder{ chosen_gpu, device, surface };
  swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

  vkb::Swapchain vkbSwapchain = swapchain_builder
    .set_desired_format(VkSurfaceFormatKHR{ .format = swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_desired_extent(width, height)
    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    .build()
    .value();

  swapchain_extent = vkbSwapchain.extent;
  swapchain = vkbSwapchain.swapchain;
  swapchain_images = vkbSwapchain.get_images().value();
  swapchain_imageviews = vkbSwapchain.get_image_views().value();
}

void Bulkin::destroy_swapchain() {
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for (auto &image_view : swapchain_imageviews)
    vkDestroyImageView(device, image_view, nullptr);
}

void Bulkin::init_swapchain() {
  create_swapchain(window_extent.width, window_extent.height);

  VkExtent3D draw_image_extent = {
    window_extent.width,
    window_extent.height,
    1
  };

  draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
  draw_image.image_extent = draw_image_extent;

  VkImageUsageFlags draw_image_usages{};
  draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
  draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkImageCreateInfo rimg_info = vkinit::image_create_info(draw_image.image_format, draw_image_usages, draw_image_extent);

  VmaAllocationCreateInfo rimg_alloc_info{};
  rimg_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vmaCreateImage(allocator, &rimg_info, &rimg_alloc_info, &draw_image.image, &draw_image.allocation, nullptr);
  VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(draw_image.image_format, draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT);

  depth_image.image_format = VK_FORMAT_D32_SFLOAT;
  depth_image.image_extent = draw_image_extent;
  VkImageUsageFlags depth_image_usages{};
  depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  VkImageCreateInfo dimg_info = vkinit::image_create_info(depth_image.image_format, depth_image_usages, draw_image_extent);

  vmaCreateImage(allocator, &dimg_info, &rimg_alloc_info, &depth_image.image, &depth_image.allocation, nullptr);
  auto dview_info = vkinit::imageview_create_info(depth_image.image_format, depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(vkCreateImageView(device, &dview_info, nullptr, &depth_image.image_view));
  VK_CHECK(vkCreateImageView(device, &rview_info, nullptr, &draw_image.image_view));

  deletion_queue.push_function([=, this]() {
    vkDestroyImageView(device, draw_image.image_view, nullptr);
    vmaDestroyImage(allocator, draw_image.image, draw_image.allocation);
    vkDestroyImageView(device, depth_image.image_view, nullptr);
    vmaDestroyImage(allocator, depth_image.image, depth_image.allocation);
  });
}

void Bulkin::init_commands() {
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  for (size_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frames[i].command_pool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frames[i].command_pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frames[i].command_buffer));
  }

  VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &imm_cmd_pool));
  VkCommandBufferAllocateInfo cmd_alloc = vkinit::command_buffer_allocate_info(imm_cmd_pool, 1);
  VK_CHECK(vkAllocateCommandBuffers(device, &cmd_alloc, &imm_cmd));

  deletion_queue.push_function([=, this]() {
    vkDestroyCommandPool(device, imm_cmd_pool, nullptr);
  });
}

void Bulkin::init_sync_structures() {
  VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

  for (size_t i = 0; i < FRAME_OVERLAP; i++) {
    VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &frames[i].render_fence));

    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frames[i].swapchain_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_create_info, nullptr, &frames[i].render_semaphore));
  }

  VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &imm_fence));
  deletion_queue.push_function([=, this]() {
    vkDestroyFence(device, imm_fence, nullptr);
  });
}

void Bulkin::init_descriptors() {
  std::vector<BulkinDescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
  };

  descriptor_allocator.init(device, 10, sizes);

  {
    BulkinDescriptorLayout layout_builder;
    layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    draw_image_descriptor_layout = layout_builder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);

    draw_image_descriptors = descriptor_allocator.allocate(device, draw_image_descriptor_layout);
    BulkinDescriptorWriter writer;
    writer.write_image(0, draw_image.image_view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.update_set(device, draw_image_descriptors);
  }

  for (size_t i = 0; i < FRAME_OVERLAP; i++) {
    std::vector<BulkinDescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}
    };

    frames[i].frame_descriptors = BulkinDescriptorAllocatorGrowable{};
    frames[i].frame_descriptors.init(device, 1000, frame_sizes);

    deletion_queue.push_function([&, i](){
      frames[i].frame_descriptors.destroy_pools(device);
    });

    {
      BulkinDescriptorLayout builder;
      builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
      single_image_descriptor_layout = builder.build(device, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
  }

  {
    BulkinDescriptorLayout builder;
    builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    gpu_scene_data_descriptor_layout = builder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    deletion_queue.push_function([&]{
      descriptor_allocator.destroy_pools(device);
      vkDestroyDescriptorSetLayout(device, draw_image_descriptor_layout, nullptr);
      vkDestroyDescriptorSetLayout(device, gpu_scene_data_descriptor_layout, nullptr);
      vkDestroyDescriptorSetLayout(device, single_image_descriptor_layout, nullptr);
    });
  }
}

void Bulkin::init_pipelines() {
  init_background_pipelines();
  init_mesh_pipeline();
  metal_material.build_pipelines(this);
}

void Bulkin::init_background_pipelines() {
  VkPipelineLayoutCreateInfo compute_layout{};
  compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  compute_layout.pNext = nullptr;
  compute_layout.pSetLayouts = &draw_image_descriptor_layout;
  compute_layout.setLayoutCount = 1;

  VkPushConstantRange push_constant{};
  push_constant.offset = 0;
  push_constant.size = sizeof(BulkinPushConstants);
  push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  compute_layout.pPushConstantRanges = &push_constant;
  compute_layout.pushConstantRangeCount = 1;

  VK_CHECK(vkCreatePipelineLayout(device, &compute_layout, nullptr, &gradient_pipeline_layout));

  VkShaderModule gradient_shader;
  if (!vkutil::load_shader_module("shaders/gradient-color.comp.spv", device, &gradient_shader)) {
    std::println("Error when building compute shader");
    exit(EXIT_FAILURE);
  }

  VkShaderModule sky_shader;
  if (!vkutil::load_shader_module("shaders/sky.comp.spv", device, &sky_shader)) {
    std::println("Error when building compute shader");
    exit(EXIT_FAILURE);
  }

  VkPipelineShaderStageCreateInfo stage_info{};
  stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stage_info.pNext = nullptr;
  stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stage_info.module = gradient_shader;
  stage_info.pName = "main";

  VkComputePipelineCreateInfo compute_pipeline_create_info{};
  compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  compute_pipeline_create_info.pNext = nullptr;
  compute_pipeline_create_info.layout = gradient_pipeline_layout;
  compute_pipeline_create_info.stage = stage_info;

  BulkinComputeEffect gradient;
  gradient.layout = gradient_pipeline_layout;
  gradient.name = "gradient";
  gradient.data = {};

  gradient.data.data1 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
  gradient.data.data2 = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

  VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &gradient.pipeline));

  compute_pipeline_create_info.stage.module = sky_shader;

  BulkinComputeEffect sky;
  sky.layout = gradient_pipeline_layout;
  sky.name = "sky";
  sky.data = {};
  sky.data.data1 = glm::vec4(0.1f, 0.2f, 0.4f, 0.97f);

  VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &sky.pipeline));

  background_effects.push_back(gradient);
  background_effects.push_back(sky);

  vkDestroyShaderModule(device, gradient_shader, nullptr);
  vkDestroyShaderModule(device, sky_shader, nullptr);

  deletion_queue.push_function([=, this]() {
    vkDestroyPipelineLayout(device, gradient_pipeline_layout, nullptr);
    vkDestroyPipeline(device, sky.pipeline, nullptr);
    vkDestroyPipeline(device, gradient.pipeline, nullptr);
  });
}

void Bulkin::run() {
  SDL_Event e;
  bool shouldQuit = false;

  while (!shouldQuit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_EVENT_QUIT) shouldQuit = true;

      if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
        stop_rendering = true;

      if (e.type == SDL_EVENT_WINDOW_RESTORED)
        stop_rendering = false;

      ImGui_ImplSDL3_ProcessEvent(&e);
    }

    if (stop_rendering) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (resize_requested)
      resize_swapchain();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("background")) {
      ImGui::SliderFloat("Render Scale", &render_scale, 0.3, 1.0f);
      BulkinComputeEffect &selected = background_effects[current_background_effect];

      ImGui::Text("Selected effect: %s", selected.name);
      ImGui::SliderInt("Effect Index", &current_background_effect, 0, background_effects.size() - 1);

      ImGui::InputFloat4("data1", (float*)&selected.data.data1);
      ImGui::InputFloat4("data2", (float*)&selected.data.data2);
      ImGui::InputFloat4("data3", (float*)&selected.data.data3);
      ImGui::InputFloat4("data4", (float*)&selected.data.data4);
    }
    ImGui::End();

    ImGui::Render();

    draw();
  }
}

void Bulkin::draw() {
  update_scene();
  int timeout = 1000000000;
  VK_CHECK(vkWaitForFences(device, 1, &get_current_frame().render_fence, true, timeout));

  get_current_frame().deletion_queue.flush();
  get_current_frame().frame_descriptors.clear_pools(device);

  VK_CHECK(vkResetFences(device, 1, &get_current_frame().render_fence));

  uint32_t swapchain_image_index;
  VkResult e = vkAcquireNextImageKHR(device, swapchain, timeout, get_current_frame().swapchain_semaphore, nullptr, &swapchain_image_index);
  if (e == VK_ERROR_OUT_OF_DATE_KHR) {
    resize_requested = true;
    return;
  }

  VkCommandBuffer cmd = get_current_frame().command_buffer;
  VK_CHECK(vkResetCommandBuffer(cmd, 0));
  VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  draw_extent.width = std::min(swapchain_extent.width, draw_image.image_extent.width) * render_scale;
  draw_extent.height = std::min(swapchain_extent.height, draw_image.image_extent.height) * render_scale;

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

  vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  draw_background(cmd);

  vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  vkutil::transition_image(cmd, depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  draw_geometry(cmd);

  vkutil::transition_image(cmd, draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkutil::transition_image(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkutil::copy_image_to_image(cmd, draw_image.image, swapchain_images[swapchain_image_index], draw_extent, swapchain_extent);

  vkutil::transition_image(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  draw_imgui(cmd, swapchain_imageviews[swapchain_image_index]);

  vkutil::transition_image(cmd, swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

  VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame().swapchain_semaphore);
  VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().render_semaphore);

  VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

  VK_CHECK(vkQueueSubmit2(graphics_queue, 1 ,&submit, get_current_frame().render_fence));

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;
  present_info.pSwapchains = &swapchain;
  present_info.swapchainCount = 1;
  present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
  present_info.waitSemaphoreCount = 1;
  present_info.pImageIndices = &swapchain_image_index;

  VkResult present_result = vkQueuePresentKHR(graphics_queue, &present_info);
  if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
    resize_requested = true;
  }

  frame_number++;

  vkQueueWaitIdle(graphics_queue);
}

void Bulkin::resize_swapchain() {
  vkDeviceWaitIdle(device);
  destroy_swapchain();
   int w, h;
   SDL_GetWindowSize(window, &w, &h);
   window_extent.width = w;
   window_extent.height = h;
   create_swapchain(w, h);
   resize_requested = false;
}

void Bulkin::draw_imgui(VkCommandBuffer cmd, VkImageView target_image_view) {
  VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(target_image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingInfo render_info = vkinit::rendering_info(swapchain_extent, &color_attachment, nullptr);
  vkCmdBeginRendering(cmd, &render_info);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  vkCmdEndRendering(cmd);
}

void Bulkin::cleanup() {
  if (isInitialized) {
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
      vkDestroyCommandPool(device, frames[i].command_pool, nullptr);

      vkDestroyFence(device, frames[i].render_fence, nullptr);
      vkDestroySemaphore(device, frames[i].swapchain_semaphore, nullptr);
      vkDestroySemaphore(device, frames[i].render_semaphore, nullptr);

      frames[i].deletion_queue.flush();
    }

    for (auto &mesh : test_meshes) {
      destroy_buffer(mesh->mesh_buffers.index_buffer);
      destroy_buffer(mesh->mesh_buffers.vertex_buffer);
    }

    metal_material.clear(device);

    deletion_queue.flush();

    destroy_swapchain();

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);

    vkb::destroy_debug_utils_messenger(instance, debug_messenger);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);
  }
  loadedEngine = nullptr;
}

void Bulkin::draw_background(VkCommandBuffer cmd) {
  BulkinComputeEffect &effect = background_effects[current_background_effect];
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradient_pipeline_layout, 0, 1, &draw_image_descriptors, 0, nullptr);


  vkCmdPushConstants(cmd, gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BulkinPushConstants), &effect.data);

  vkCmdDispatch(cmd, std::ceil(draw_extent.width / 16.0), std::ceil(draw_extent.height / 16.0), 1);
}

void Bulkin::imm_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
  VK_CHECK(vkResetFences(device, 1, &imm_fence));
  VK_CHECK(vkResetCommandBuffer(imm_cmd, 0));

  VkCommandBuffer cmd = imm_cmd;
  VkCommandBufferBeginInfo begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));
  function(cmd);
  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);
  VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, nullptr, nullptr);

  VK_CHECK(vkQueueSubmit2(graphics_queue, 1, &submit, imm_fence));
  VK_CHECK(vkWaitForFences(device, 1, &imm_fence, true, 999999999999));
}

void Bulkin::init_imgui() {
  VkDescriptorPoolSize pool_sizes[] = {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VkDescriptorPool imgui_pool;
  VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_pool));

  ImGui::CreateContext();
  ImGui_ImplSDL3_InitForVulkan(window);

  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = instance;
  init_info.PhysicalDevice = chosen_gpu;
  init_info.Device = device;
  init_info.Queue = graphics_queue;
  init_info.DescriptorPool = imgui_pool;
  init_info.MinImageCount = 3;
  init_info.ImageCount = 3;
  init_info.UseDynamicRendering = true;

  init_info.PipelineInfoMain.PipelineRenderingCreateInfo ={ .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

  init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  ImGui_ImplVulkan_Init(&init_info);

  deletion_queue.push_function([=, this] () {
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(device, imgui_pool, nullptr);
  });
}

void Bulkin::draw_geometry(VkCommandBuffer cmd) {
  VkRenderingAttachmentInfo color_attachment = vkinit::attachment_info(draw_image.image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingAttachmentInfo depth_attachment = vkinit::depth_attachment_info(depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
  VkRenderingInfo render_info = vkinit::rendering_info(draw_extent, &color_attachment, &depth_attachment);
  vkCmdBeginRendering(cmd, &render_info);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = draw_extent.width;
  viewport.height = draw_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = draw_extent.width;
  scissor.extent.height = draw_extent.height;

  vkCmdSetScissor(cmd, 0, 1, &scissor);

  VkDescriptorSet image_set = get_current_frame().frame_descriptors.allocate(device, single_image_descriptor_layout);
  {
    BulkinDescriptorWriter writer;
    writer.write_image(0, error_checkerboard_image.image_view, default_sampler_nearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.update_set(device, image_set);
  }

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh_pipeline_layout, 0, 1, &image_set, 0, nullptr);

  BulkinBuffer gpu_scene_data_buffer = create_buffer(sizeof(BulkinGPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  get_current_frame().deletion_queue.push_function([=, this](){
    destroy_buffer(gpu_scene_data_buffer);
  });

  BulkinGPUSceneData *scene_uniform_data = (BulkinGPUSceneData*)gpu_scene_data_buffer.allocation->GetMappedData();
  *scene_uniform_data = scene_data;

  VkDescriptorSet global_descriptor = get_current_frame().frame_descriptors.allocate(device, gpu_scene_data_descriptor_layout);

  BulkinDescriptorWriter writer;
  writer.write_buffer(0, gpu_scene_data_buffer.buffer, sizeof(BulkinGPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.update_set(device, global_descriptor);

  for (const BulkinRenderObject &draw : main_draw_context.opaque_surfaces) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1, &global_descriptor, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1, &draw.material->materialSet, 0, nullptr);
    vkCmdBindIndexBuffer(cmd, draw.index_buffer, 0, VK_INDEX_TYPE_UINT32);

    BulkinDrawPushConstants pc;
    pc.vertex_buffer = draw.vertex_buffer_address;
    pc.world_matrix = draw.transform;
    vkCmdPushConstants(cmd, draw.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BulkinDrawPushConstants), &pc);
    vkCmdDrawIndexed(cmd, draw.index_count, 1, draw.first_index, 0, 0);
  }

  vkCmdEndRendering(cmd);
}

BulkinBuffer Bulkin::create_buffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.size = alloc_size;
  buffer_info.usage = usage;

  VmaAllocationCreateInfo vma_alloc_info = {};
  vma_alloc_info.usage = memory_usage;
  vma_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  BulkinBuffer new_buffer;

  VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_alloc_info, &new_buffer.buffer, &new_buffer.allocation, &new_buffer.info));

  return new_buffer;
}

void Bulkin::destroy_buffer(const BulkinBuffer &buffer) {
  vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

BulkinMeshBuffer Bulkin::upload_mesh(std::span<uint32_t> indices, std::span<BulkinVertex> vertices) {
  const size_t vertex_buffer_size = vertices.size() * sizeof(BulkinVertex);
  const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

  BulkinMeshBuffer new_surface;
  new_surface.vertex_buffer = create_buffer(vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  VkBufferDeviceAddressInfo device_address_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = new_surface.vertex_buffer.buffer};
  new_surface.vertex_buffer_address = vkGetBufferDeviceAddress(device, &device_address_info);

  new_surface.index_buffer = create_buffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  BulkinBuffer staging = create_buffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
  void *data = staging.allocation->GetMappedData();
  memcpy(data, vertices.data(), vertex_buffer_size);
  memcpy((char*)data + vertex_buffer_size, indices.data(), index_buffer_size);

  imm_submit([&](VkCommandBuffer cmd) {
    VkBufferCopy vertex_copy{0};
    vertex_copy.dstOffset = 0;
    vertex_copy.srcOffset = 0;
    vertex_copy.size = vertex_buffer_size;

    vkCmdCopyBuffer(cmd, staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertex_copy);

    VkBufferCopy index_copy{0};
    index_copy.dstOffset = 0;
    index_copy.srcOffset = vertex_buffer_size;
    index_copy.size = index_buffer_size;

    vkCmdCopyBuffer(cmd, staging.buffer, new_surface.index_buffer.buffer, 1, &index_copy);
  });

  destroy_buffer(staging);

  return new_surface;
}

void Bulkin::init_mesh_pipeline() {
  VkShaderModule mesh_vertex_shader, mesh_fragment_shader;
  if (!vkutil::load_shader_module("shaders/colored-triangle-mesh.vert.spv", device, &mesh_vertex_shader) ||
    !vkutil::load_shader_module("shaders/tex-image.frag.spv", device, &mesh_fragment_shader)) {
    std::println("unable to load shaders");
    exit(EXIT_FAILURE);
  }

  VkPushConstantRange buffer_range{};
  buffer_range.offset = 0;
  buffer_range.size = sizeof(BulkinDrawPushConstants);
  buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
  pipeline_layout_info.pPushConstantRanges = &buffer_range;
  pipeline_layout_info.pushConstantRangeCount = 1;
  pipeline_layout_info.pSetLayouts = &single_image_descriptor_layout;
  pipeline_layout_info.setLayoutCount = 1;
  VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &mesh_pipeline_layout));

  BulkinPipeline pipeline_builder;
  pipeline_builder.pipeline_layout = mesh_pipeline_layout;
  pipeline_builder.set_shaders(mesh_vertex_shader, mesh_fragment_shader);
  pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  pipeline_builder.set_multisampling_none();
  pipeline_builder.disable_blending();
  pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
  pipeline_builder.set_color_attachment_format(draw_image.image_format);
  pipeline_builder.set_depth_format(depth_image.image_format);
  mesh_pipeline = pipeline_builder.build_pipeline(device);
  vkDestroyShaderModule(device, mesh_vertex_shader, nullptr);
  vkDestroyShaderModule(device, mesh_fragment_shader, nullptr);

  deletion_queue.push_function([=, this]() {
    vkDestroyPipelineLayout(device, mesh_pipeline_layout, nullptr);
    vkDestroyPipeline(device, mesh_pipeline, nullptr);
  });
}

void Bulkin::init_default_data() {
  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
  white_image = create_image((void*)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1.0f));
  grey_image = create_image((void*)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
  black_image = create_image((void*)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  //checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	error_checkerboard_image = create_image(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;
	vkCreateSampler(device, &sampl, nullptr, &default_sampler_nearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(device, &sampl, nullptr, &default_sampler_linear);

	deletion_queue.push_function([&](){
  	vkDestroySampler(device, default_sampler_nearest, nullptr);
  	vkDestroySampler(device, default_sampler_linear, nullptr);

  	destroy_image(white_image);
  	destroy_image(grey_image);
  	destroy_image(black_image);
  	destroy_image(error_checkerboard_image);
	});

  test_meshes = loadGltfMeshes(this, "resources/basicmesh.glb").value();

  BulkinGLTFMetallic_Roughness::MaterialResources material_resources;
  material_resources.color_image = white_image;
  material_resources.color_sampler = default_sampler_linear;
  material_resources.metal_rough_image = white_image;
  material_resources.metal_rough_sampler = default_sampler_linear;

  BulkinBuffer material_constants = create_buffer(sizeof(BulkinGLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  BulkinGLTFMetallic_Roughness::MaterialConstants *scene_uniform_data = (BulkinGLTFMetallic_Roughness::MaterialConstants*)material_constants.allocation->GetMappedData();
  scene_uniform_data->color_factors = glm::vec4{1, 1, 1, 1};
  scene_uniform_data->metal_rough_factors = glm::vec4{1, 0.5, 0, 0};

  deletion_queue.push_function([=, this]() { destroy_buffer(material_constants); });

  material_resources.data_buffer = material_constants.buffer;
  material_resources.data_buffer_offset = 0;

  default_data = metal_material.write_material(device, BulkinMaterialPass::MainColor, material_resources, descriptor_allocator);

  for (auto &m : test_meshes) {
    std::shared_ptr<BulkinMeshNode> new_node = std::make_shared<BulkinMeshNode>();
    new_node->mesh = m;
    new_node->local_transform = glm::mat4{ 1.0f };
    new_node->world_transform = glm::mat4{ 1.0f };
    for (auto & s : new_node->mesh->surfaces) {
      s.material = std::make_shared<BulkinGLTFMaterial>(default_data);
    }
    loaded_nodes[m->name] = std::move(new_node);
  }
}

BulkinImage Bulkin::create_image(VkExtent3D size, VkFormat format, VkBufferUsageFlags usage, bool mipmapped) {
  BulkinImage new_image;
  new_image.image_format = format;
  new_image.image_extent = size;

  VkImageCreateInfo img_info = vkinit::image_create_info(format, usage, size);
  if (mipmapped) {
    img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
  }

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_CHECK(vmaCreateImage(allocator, &img_info, &alloc_info, &new_image.image, &new_image.allocation, nullptr));

  VkImageAspectFlags aspect_flag = VK_IMAGE_ASPECT_COLOR_BIT;
  if (format == VK_FORMAT_D32_SFLOAT) {
    aspect_flag = VK_IMAGE_ASPECT_DEPTH_BIT;
  }

  VkImageViewCreateInfo view_info = vkinit::imageview_create_info(format, new_image.image, aspect_flag);
  view_info.subresourceRange.layerCount = img_info.mipLevels;

  VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &new_image.image_view));

  return new_image;
}

BulkinImage Bulkin::create_image(void *data, VkExtent3D size, VkFormat format, VkBufferUsageFlags usage, bool mipmapped) {
  size_t data_size = size.depth * size.width * size.height * 4;
  BulkinBuffer upload_buffer = create_buffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  memcpy(upload_buffer.info.pMappedData, data, data_size);

  BulkinImage new_image = create_image(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, mipmapped);

  imm_submit([&](VkCommandBuffer cmd){
    vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy copy = {};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageExtent = size;

    vkCmdCopyBufferToImage(cmd, upload_buffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    vkutil::transition_image(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  });

  destroy_buffer(upload_buffer);

  return new_image;
}

void Bulkin::destroy_image(const BulkinImage &img) {
  vkDestroyImageView(device, img.image_view, nullptr);
  vmaDestroyImage(allocator, img.image, img.allocation);
}

void Bulkin::update_scene() {
  main_draw_context.opaque_surfaces.clear();
  loaded_nodes["Suzanne"]->draw(glm::mat4{1.0f}, main_draw_context);
  scene_data.view = glm::translate(glm::vec3{0, 0, -5});
  scene_data.proj = glm::perspective(glm::radians(70.0f), (float)window_extent.width/(float)window_extent.height, 0.0f, 10000.0f);
  scene_data.proj[1][1] *= -1;
  scene_data.viewproj = scene_data.proj * scene_data.view;
  scene_data.ambient_color = glm::vec4(0.1f);
  scene_data.sunlight_color = glm::vec4(1.0f);
  scene_data.sunlight_dir = glm::vec4(0.0f, 1.0f, 0.5f, 1.0f);

  for (int x = -3; x < 3; x++) {
    glm::mat4 scale = glm::scale(glm::vec3{0.2f});
    glm::mat4 translation = glm::translate(glm::vec3{x, 1, 0});

    loaded_nodes["Cube"]->draw(translation * scale, main_draw_context);
  }
}
