#include "bulkin-material.hpp"
#include "bulkin.hpp"
#include "bulkin-pipelines.hpp"
#include "vk_initializers.h"

void BulkinGLTFMetallic_Roughness::build_pipelines(Bulkin *app) {
  VkShaderModule mesh_frag_shader, mesh_vert_shader;
  if (!vkutil::load_shader_module("shaders/mesh.frag.spv", app->device, &mesh_frag_shader) || !vkutil::load_shader_module("shaders/mesh.vert.spv", app->device, &mesh_vert_shader)) {
    std::println("unable to load mesh shader modules");
    exit(EXIT_FAILURE);
  }

  VkPushConstantRange matrix_range{};
  matrix_range.offset = 0;
  matrix_range.size = sizeof(BulkinDrawPushConstants);
  matrix_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  BulkinDescriptorLayout layout_builder;
  layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  layout_builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  layout_builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  material_layout = layout_builder.build(app->device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  VkDescriptorSetLayout layouts[] = {app->gpu_scene_data_descriptor_layout, material_layout};

  VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
  mesh_layout_info.setLayoutCount = 2;
  mesh_layout_info.pSetLayouts = layouts;
  mesh_layout_info.pPushConstantRanges = &matrix_range;
  mesh_layout_info.pushConstantRangeCount = 1;

  VkPipelineLayout new_layout;
  VK_CHECK(vkCreatePipelineLayout(app->device, &mesh_layout_info, nullptr, &new_layout));

  opaque_pipeline.layout = new_layout;
  transparent_pipeline.layout = new_layout;

  BulkinPipeline pipeline_builder;
  pipeline_builder.set_shaders(mesh_vert_shader, mesh_frag_shader);
  pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  pipeline_builder.set_multisampling_none();
  pipeline_builder.disable_blending();
  pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
  pipeline_builder.set_color_attachment_format(app->draw_image.image_format);
  pipeline_builder.set_depth_format(app->depth_image.image_format);
  pipeline_builder.pipeline_layout = new_layout;
  opaque_pipeline.pipeline = pipeline_builder.build_pipeline(app->device);
  pipeline_builder.enable_blending_additive();
  pipeline_builder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
  transparent_pipeline.pipeline = pipeline_builder.build_pipeline(app->device);
  vkDestroyShaderModule(app->device, mesh_vert_shader, nullptr);
  vkDestroyShaderModule(app->device, mesh_frag_shader, nullptr);
}

void BulkinGLTFMetallic_Roughness::clear(VkDevice device) {
  vkDestroyDescriptorSetLayout(device, material_layout, nullptr);
  vkDestroyPipelineLayout(device, transparent_pipeline.layout, nullptr);
  vkDestroyPipeline(device, transparent_pipeline.pipeline, nullptr);
  vkDestroyPipeline(device, opaque_pipeline.pipeline, nullptr);
}

BulkinMaterial BulkinGLTFMetallic_Roughness::write_material(VkDevice device, BulkinMaterialPass pass, const MaterialResources &resources, BulkinDescriptorAllocatorGrowable &descriptor_allocator) {
  BulkinMaterial mat_data;
  mat_data.passType = pass;
  if (pass == BulkinMaterialPass::Transparent) {
    mat_data.pipeline = &transparent_pipeline;
  } else {
    mat_data.pipeline = &opaque_pipeline;
  }

  mat_data.materialSet = descriptor_allocator.allocate(device, material_layout);

  writer.clear();
  writer.write_buffer(0, resources.data_buffer, sizeof(MaterialConstants), resources.data_buffer_offset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.write_image(1, resources.color_image.image_view, resources.color_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  writer.write_image(2, resources.metal_rough_image.image_view, resources.metal_rough_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  writer.update_set(device, mat_data.materialSet);

  return mat_data;
}
