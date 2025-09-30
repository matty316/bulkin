#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

class BulkinCamera {
public:
  BulkinCamera() = default;
  BulkinCamera(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up) : cameraPos(pos), front(front), up(up), worldUp(up) {
    setCameraVectors();
  }
  void update(double deltaTime, const glm::vec2& mousePos, bool mousePressed);
  glm::mat4 getView();
  glm::vec3 getPosition();
  
  struct Movement {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool fast = false;
  } movement;
  
private:
  float mouseSpeed = 10.0f;
  float acceleration = 150.0f;
  float damping = 0.2f;
  float maxSpeed = 5.0f;
  float fastCoef = 10.0f;
  float yaw = -90.0f;
  float pitch = 0.0f;
  glm::vec2 mousePosition = glm::vec2(0.0f);
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 moveSpeed = glm::vec3(0.0f);
  glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 right = glm::vec3(0.0f);
  glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);

  void setPosition(const glm::vec3& pos);
  void setCameraVectors();
  void resetMousePosition(const glm::vec2& p);
};
