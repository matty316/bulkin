#pragma once

#include "bulkin-vertex.hpp"
#include <unordered_map>
#include <filesystem>
#include <vector>

struct BulkinSurface {
  uint32_t start_index;
  uint32_t count;
};

struct BulkinMeshAsset {
  std::string name;
  std::vector<BulkinSurface> surfaces;
  BulkinMeshBuffer mesh_buffers;
};

class Bulkin;

std::optional<std::vector<std::shared_ptr<BulkinMeshAsset>>> loadGltfMeshes(Bulkin *app, std::filesystem::path filepath);
