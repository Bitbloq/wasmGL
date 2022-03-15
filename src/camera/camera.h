#ifndef CAMERA_H
#define CAMERA_H

#include <memory>
#include <GL/glew.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera
{
public:
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = 0.0f, GLfloat pitch = 0.0f, GLfloat movementSpeed = 0.05f, GLfloat turnSpeed = 0.05f);
  ~Camera();

  void keyControl(std::shared_ptr<std::array<bool, 1024>> keys, GLfloat deltaTime);
  glm::mat4 calculateViewMatrix();

private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 worldUp;

  GLfloat yaw;
  GLfloat pitch;

  GLfloat movementSpeed;
  GLfloat turnSpeed;

  void udpate();
};
#endif