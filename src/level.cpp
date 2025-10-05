#include "level.h"

#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <iostream>

BulkinLevel::BulkinLevel(const char *path) {
  tmx::Map map;
  if (map.load(path)) {
    const auto& layers = map.getLayers();
    for (const auto& layer : layers) {
      if (layer->getName() == "walls") {
        const auto& tiles = layer->getLayerAs<tmx::TileLayer>().getTiles();
        height = layer->getSize().y;
        width = layer->getSize().x;
        walls.resize(height);
        for (size_t z = 0; z < height; z++) {
          walls[z].resize(width);
          for (size_t x = 0; x < width; x++) {
            walls[z][x] = tiles[z * width + x].ID;
          }
        }
      } else if (layer->getName() == "player") {
        const auto& position = layer->getLayerAs<tmx::ObjectGroup>().getObjects().front().getPosition();
        playerPos.x = position.x / map.getTileSize().x;
        playerPos.y = position.y / map.getTileSize().y;
      }
    }
  } else {
    std::runtime_error("failed to load level");
  }
}

void BulkinLevel::renderLevel(Bulkin& app) {
  app.setPlayerPos(playerPos);
  for (size_t z = 0; z < height; z++) {
    for (size_t x = 0; x < width; x++) {
      auto wall = walls[z][x];
      if (wall == 1) {
        if (z != height - 1 && walls[z + 1][x] == 0)
          app.addQuad(glm::vec3(0.0f + x, 0.0f, 0.0f + z), 0.0f, 0.0f, 0.0f, 1.0f, 2);
        if (x != width - 1 && walls[z][x + 1] == 0)
          app.addQuad(glm::vec3(0.5f + x, 0.0f, -0.5f + z), 0.0f, 90.0f, 0.0f, 1.0f, 3);
        if (x != 0 && walls[z][x - 1] == 0)
          app.addQuad(glm::vec3(-0.5f + x, 0.0f, -0.5f + z), 0.0f, 270.0f, 0.0f, 1.0f, 4);
        if (z != 0 && walls[z - 1][x] == 0)
          app.addQuad(glm::vec3(0.0f + x, 0.0f, -1.0f + z), 0.0f, 180.0f, 0.0f, 1.0f, 5);
      }
    }
  }
}
