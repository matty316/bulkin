#pragma once

#include <glm/glm.hpp>
#include "bulkin-loader.hpp"
#include "bulkin-scene.hpp"

struct BulkinDrawContext {
  std::vector<BulkinRenderObject> opaque_surfaces;
};

class BulkinRenderable {
  virtual void draw(const glm::mat4& top_matrix, BulkinDrawContext& ctx) = 0;
};

struct BulkinNode: public BulkinRenderable {
  std::weak_ptr<BulkinNode> parent;
  std::vector<std::shared_ptr<BulkinNode>> children;
  glm::mat4 local_transform;
  glm::mat4 world_transform;

  void refresh_transform(const glm::mat4 &parent_matrix) {
    world_transform = parent_matrix * local_transform;
    for (auto c : children) {
      c->refresh_transform(world_transform);
    }
  }

  virtual void draw(const glm::mat4 &top_matrix, BulkinDrawContext &ctx) {
    for (auto &c : children) {
      c->draw(top_matrix, ctx);
    }
  }
};

struct BulkinMeshNode : public BulkinNode {
  std::shared_ptr<BulkinMeshAsset> mesh;
  virtual void draw(const glm::mat4 &top_matrix, BulkinDrawContext &ctx) override;
};
