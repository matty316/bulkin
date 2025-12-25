#include "bulkin-rendering.hpp"

void BulkinMeshNode::draw(const glm::mat4 &top_matrix, BulkinDrawContext &ctx) {
  glm::mat4 node_matrix = top_matrix * world_transform;

  for (auto &s : mesh->surfaces) {
    BulkinRenderObject def;
    def.index_count = s.count;
    def.first_index = s.start_index;
    def.index_buffer = mesh->mesh_buffers.index_buffer.buffer;
    def.material = &s.material->data;

    def.transform = node_matrix;
    def.vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address;

    ctx.opaque_surfaces.push_back(def);
  }

  BulkinNode::draw(top_matrix, ctx);
}
