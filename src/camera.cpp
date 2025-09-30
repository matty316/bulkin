#include "camera.h"

void BulkinCamera::update(double deltaTime, const glm::vec2 &mousePos) {
  if (abs(mousePosition.x - mousePos.x) + abs(mousePosition.y - mousePos.y) > FLT_EPSILON) {
    auto xoffset = mousePos.x * mouseSpeed;
    auto yoffset = mousePos.y * mouseSpeed;
    
    yaw += xoffset;
    pitch += yoffset;
    
    if (pitch > 89.0f)
      pitch = 89.0f;
    if (pitch < -89.0f)
      pitch = -89.0f;
    
    setCameraVectors();
    mousePosition = mousePos;
  }
  
  glm::vec3 accel(0.0f);
  if (movement.forward) accel += front;
  if (movement.backward) accel -= front;
  if (movement.left) accel -= right;
  if (movement.right) accel += right;
  if (movement.up) accel += up;
  if (movement.down) accel -= up;
  if (movement.fast) accel *= fastCoef;
  
  if (accel == glm::vec3(0.0f)) {
    moveSpeed -= moveSpeed * glm::min((1.0f / damping) * static_cast<float>(deltaTime), 1.0f);
  } else {
    moveSpeed += accel * acceleration * static_cast<float>(deltaTime);
    const float maximumSpeed = movement.fast ? maxSpeed * fastCoef : maxSpeed;
    if (glm::length(moveSpeed) > maximumSpeed)
      moveSpeed = glm::normalize(moveSpeed) * maximumSpeed;
  }
  cameraPos += moveSpeed * static_cast<float>(deltaTime);
}

glm::mat4 BulkinCamera::getView() {
  return glm::lookAt(cameraPos, cameraPos + front, up);
}

glm::vec3 BulkinCamera::getPosition() {
  return cameraPos;
}

void BulkinCamera::setPosition(const glm::vec3 &pos) {
  cameraPos = pos;
}

void BulkinCamera::setCameraVectors() {
  glm::vec3 newFront;
  newFront.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  newFront.y = glm::sin(glm::radians(pitch));
  newFront.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  front = glm::normalize(newFront);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}

void BulkinCamera::resetMousePosition(const glm::vec2 &p) {
  mousePosition = p;
}
