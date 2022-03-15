#include "camera.h"
#include <GLFW/glfw3.h>
#include <array>

Camera::Camera(glm::vec3 position,
               glm::vec3 up,
               GLfloat yaw,
               GLfloat pitch,
               GLfloat movementSpeed,
               GLfloat turnSpeed) : position{position},
                                    front{glm::vec3(0.0f, 0.0f, -1.0f)},
                                    worldUp{up},
                                    yaw{yaw},
                                    pitch{pitch},
                                    movementSpeed{movementSpeed},
                                    turnSpeed{turnSpeed}

{
  udpate();
}

Camera::~Camera()
{
}

glm::mat4 Camera::calculateViewMatrix()
{
  return glm::lookAt(position, position + front, up);
}

void Camera::keyControl(std::shared_ptr<std::array<bool, 1024>> keys, GLfloat deltaTime)
{
  if (keys->at(GLFW_KEY_W))
  {
    position += front * movementSpeed * deltaTime;
  }
  if (keys->at(GLFW_KEY_S))
  {
    position -= front * movementSpeed * deltaTime;
  }
  if (keys->at(GLFW_KEY_A))
  {
    position -= right * movementSpeed * deltaTime;
  }
  if (keys->at(GLFW_KEY_D))
  {
    position += right * movementSpeed * deltaTime;
  }
}

void Camera::mouseControl(GLfloat xChange, GLfloat yChange)
{
  xChange *= turnSpeed;
  yChange *= turnSpeed;

  yaw += xChange;
  pitch += yChange;

  if (pitch > 89.0f)
  {
    pitch = 89.0f;
  }
  if (pitch < -89.0f)
  {
    pitch = -89.0f;
  }

  udpate();
}

void Camera::udpate()
{
  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  this->front = glm::normalize(front);

  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}