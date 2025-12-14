#include "bulkin-loader.hpp"

#include <stb_image.h>
#include <print>
#include <optional>
#include "bulkin.hpp"
#include "vk_types.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

std::optional<std::vector<std::shared_ptr<BulkinMeshAsset>>> loadGltfMeshes(Bulkin *app, std::filesystem::path filepath) {
  std::println("Loading GLTF: {}", std::string(filepath));

  auto data = fastgltf::GltfDataBuffer::FromPath(filepath);

  if (data.get_if() == nullptr) {
    std::println("unable to load gltf {}", std::string(filepath));
    exit(EXIT_FAILURE);
  }

  constexpr auto gltfops = fastgltf::Options::LoadExternalBuffers;
  fastgltf::Asset gltf;
  fastgltf::Parser parser{};
  auto load = parser.loadGltfBinary(data.get(), filepath, gltfops);
  if (load) {
    gltf = std::move(load.get());
  } else {
    std::println("unable to load gltf {}", std::string(filepath));
    exit(EXIT_FAILURE);
  }

  std::vector<std::shared_ptr<BulkinMeshAsset>> meshes;
  std::vector<uint32_t> indices;
  std::vector<BulkinVertex> vertices;

  for (auto &mesh: gltf.meshes) {
    BulkinMeshAsset new_mesh;
    new_mesh.name = mesh.name;

    indices.clear();
    vertices.clear();

    for (auto &&p : mesh.primitives) {
      BulkinSurface new_surface;
      new_surface.start_index = (uint32_t)indices.size();
      new_surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      {
        fastgltf::Accessor &index_accessor = gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + index_accessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(gltf, index_accessor, [&](std::uint32_t idx) {
          indices.push_back(idx + initial_vtx);
        });
      }

      {
        fastgltf::Accessor &pos_accessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + pos_accessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, pos_accessor, [&](glm::vec3 v, size_t index){
          BulkinVertex newvtx;
          newvtx.position = v;
          newvtx.normal = {1, 0, 0};
          newvtx.color = glm::vec4 {1.f};
          newvtx.uv_x = 0;
          newvtx.uv_y = 0;
          vertices[initial_vtx + index] = newvtx;
        });
      }

      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex], [&](glm::vec3 v, size_t index){
          vertices[initial_vtx + index].normal = v;
        });
      }

      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end()) {
        fastgltf::iterateAccessor<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex], [&](glm::vec2 v, size_t index){
          vertices[initial_vtx + index].uv_x = v.x;
          vertices[initial_vtx + index].uv_y = v.y;
        });
      }

      auto color = p.findAttribute("COLOR_0");
      if (color != p.attributes.end()) {
        fastgltf::iterateAccessor<glm::vec4>(gltf, gltf.accessors[(*color).accessorIndex], [&](glm::vec4 v, size_t index){
          vertices[initial_vtx + index].color = v;
        });
      }

      new_mesh.surfaces.push_back(new_surface);
    }

    constexpr bool override_colors = true;
    if (override_colors) {
      for (BulkinVertex &vtx : vertices) {
        vtx.color = glm::vec4(vtx.normal, 1.0f);
      }
    }
    new_mesh.mesh_buffers = app->upload_mesh(indices, vertices);
    meshes.emplace_back(std::make_shared<BulkinMeshAsset>(std::move(new_mesh)));
  }

  return meshes;
}
