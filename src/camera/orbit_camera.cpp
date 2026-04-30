#include "orbit_camera.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include "glm/gtc/matrix_transform.hpp"

namespace
{
constexpr float kEpsilon = 0.001f;
constexpr float kWheelZoomBase = 0.7f;
constexpr float kButtonZoomBase = 0.95f;

inline float dampScale(GLfloat deltaTime, float damping)
{
  return (damping * deltaTime) / 0.016f;
}
} // namespace

void OrbitCamera::Spherical::setFromOffset(glm::vec3 const &v)
{
  float const len = glm::length(v);
  radius = std::max(len, 1e-6f);
  /** Azimuth in XY around +Z; polar angle from +Z (grid normal). */
  theta = std::atan2(v.y, v.x);
  phi = std::acos(std::clamp(v.z / radius, -1.0f, 1.0f));
  makeSafe();
}

void OrbitCamera::Spherical::makeSafe()
{
  float const eps = kEpsilon;
  phi = std::clamp(phi, eps, static_cast<float>(M_PI) - eps);
}

glm::vec3 OrbitCamera::Spherical::cartesian() const
{
  float const sp = std::sin(phi);
  return glm::vec3(
      radius * sp * std::cos(theta),
      radius * sp * std::sin(theta),
      radius * std::cos(phi));
}

OrbitCamera::OrbitCamera(glm::vec3 const &worldPosition, glm::vec3 const &target)
    : target_{target}, targetEnd_{target}
{
  glm::vec3 const offset = worldPosition - target_;
  spherical_.setFromOffset(offset);
  sphericalEnd_ = spherical_;
  position_ = target_ + spherical_.cartesian();
  update(0.0f);
}

glm::mat4 OrbitCamera::calculateViewMatrix() const
{
  return glm::lookAt(position_, target_, glm::vec3(0.0f, 0.0f, 1.0f));
}

bool OrbitCamera::update(GLfloat deltaTime)
{
  float const damp = dampScale(deltaTime, dragging_ ? draggingDampingFactor_ : dampingFactor_);

  float const dTheta = sphericalEnd_.theta - spherical_.theta;
  float const dPhi = sphericalEnd_.phi - spherical_.phi;
  float const dRadius = sphericalEnd_.radius - spherical_.radius;
  glm::vec3 const dTarget = targetEnd_ - target_;

  bool moving = std::abs(dTheta) > kEpsilon || std::abs(dPhi) > kEpsilon ||
                std::abs(dRadius) > kEpsilon || std::abs(dTarget.x) > kEpsilon ||
                std::abs(dTarget.y) > kEpsilon || std::abs(dTarget.z) > kEpsilon;

  if (moving)
  {
    spherical_.radius += dRadius * damp;
    spherical_.phi += dPhi * damp;
    spherical_.theta += dTheta * damp;
    target_ += dTarget * damp;
    needsUpdate_ = true;
  }
  else
  {
    spherical_.theta = sphericalEnd_.theta;
    spherical_.phi = sphericalEnd_.phi;
    spherical_.radius = sphericalEnd_.radius;
    target_ = targetEnd_;
  }

  spherical_.makeSafe();

  position_ = target_ + spherical_.cartesian();

  bool const nu = needsUpdate_;
  needsUpdate_ = false;
  return nu;
}

void OrbitCamera::rotateTo(float theta, float phi, bool enableTransition)
{
  float const t = std::clamp(theta, minAzimuthAngle_, maxAzimuthAngle_);
  float const p = std::clamp(phi, minPolarAngle_, maxPolarAngle_);
  sphericalEnd_.theta = t;
  sphericalEnd_.phi = p;
  sphericalEnd_.makeSafe();
  if (!enableTransition)
  {
    spherical_.theta = sphericalEnd_.theta;
    spherical_.phi = sphericalEnd_.phi;
  }
  needsUpdate_ = true;
}

void OrbitCamera::rotate(float rotTheta, float rotPhi, bool enableTransition)
{
  rotateTo(sphericalEnd_.theta + rotTheta, sphericalEnd_.phi + rotPhi, enableTransition);
}

void OrbitCamera::dollyTo(float distance, bool enableTransition)
{
  sphericalEnd_.radius = std::clamp(distance, minDistance_, maxDistance_);
  if (!enableTransition)
    spherical_.radius = sphericalEnd_.radius;
  needsUpdate_ = true;
}

void OrbitCamera::dolly(float distance, bool enableTransition)
{
  dollyTo(sphericalEnd_.radius + distance, enableTransition);
}

void OrbitCamera::truck(float x, float y, bool enableTransition)
{
  glm::vec3 const forward = glm::normalize(target_ - position_);
  glm::vec3 worldUp(0.0f, 0.0f, 1.0f);
  glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
  if (glm::length(right) < 1e-6f)
    right = glm::vec3(1.0f, 0.0f, 0.0f);
  glm::vec3 up = glm::normalize(glm::cross(right, forward));

  glm::vec3 const offset = right * x + up * (-y);
  targetEnd_ += offset;
  if (!enableTransition)
    target_ = targetEnd_;
  needsUpdate_ = true;
}

void OrbitCamera::applyRotatePixels(GLfloat xChange, GLfloat yChange, GLfloat canvasW, GLfloat canvasH)
{
  if (canvasW < 1.0f)
    canvasW = 1.0f;
  if (canvasH < 1.0f)
    canvasH = 1.0f;
  // Match TS: deltaX = dragStart.x - x → rotTheta uses negative of screen-space +x when using xPos-lastX
  float const rotTheta = (-2.0f * static_cast<float>(M_PI) * xChange) / canvasW;
  float const rotPhi = (2.0f * static_cast<float>(M_PI) * yChange) / canvasH;
  rotate(rotTheta, rotPhi, true);
}

void OrbitCamera::applyPanPixels(GLfloat xChange, GLfloat yChange, GLfloat canvasW, GLfloat canvasH)
{
  if (canvasH < 1.0f)
    canvasH = 1.0f;
  glm::vec3 const offset = position_ - target_;
  float const dist = glm::length(offset);
  float targetDistance = dist * 0.5f;
  if (fovDegrees_ > 1.0f)
  {
    float const fovRad = glm::radians(fovDegrees_);
    targetDistance = dist * std::tan(fovRad * 0.5f);
  }
  // Match TS: deltaX = dragStart.x - x → move right gives negative deltaX
  float const panX = -truckSpeed_ * xChange * targetDistance / canvasH;
  float const panY = truckSpeed_ * yChange * targetDistance / canvasH;
  truck(panX, panY, true);
}

void OrbitCamera::applyWheel(GLfloat yScrollOffset)
{
  float const zoomScale = std::pow(kWheelZoomBase, dollySpeed_);
  if (yScrollOffset < 0.0f)
  {
    // Zoom in (closer): smaller radius
    float const next = sphericalEnd_.radius * zoomScale - sphericalEnd_.radius;
    dolly(next, true);
  }
  else if (yScrollOffset > 0.0f)
  {
    float const next = sphericalEnd_.radius / zoomScale - sphericalEnd_.radius;
    dolly(next, true);
  }
}

void OrbitCamera::zoomInButton()
{
  float const zoomScale = std::pow(kButtonZoomBase, dollySpeed_);
  dolly(sphericalEnd_.radius * zoomScale - sphericalEnd_.radius, true);
}

void OrbitCamera::zoomOutButton()
{
  float const zoomScale = std::pow(kButtonZoomBase, dollySpeed_);
  dolly(sphericalEnd_.radius / zoomScale - sphericalEnd_.radius, true);
}

void OrbitCamera::nudgeViewYawDegrees(GLfloat deltaYawDeg)
{
  rotate(glm::radians(deltaYawDeg), 0.0f, true);
}

void OrbitCamera::nudgeViewPitchDegrees(GLfloat deltaPitchDeg)
{
  rotate(0.0f, glm::radians(deltaPitchDeg), true);
}

void OrbitCamera::nudgePositionView(GLfloat alongFront, GLfloat alongRight, GLfloat alongUp)
{
  // Forward along view moves toward target → decrease orbit radius
  dolly(-alongFront, true);
  truck(alongRight, alongUp, true);
}

void OrbitCamera::snapOrbitToAngles(float theta, float phi, bool smoothTransition)
{
  rotateTo(theta, phi, smoothTransition);
}

void OrbitCamera::keyControl(std::shared_ptr<std::array<bool, 1024>> keys, GLfloat deltaTime)
{
  float const k = movementSpeed_ * deltaTime;
  if (keys->at(GLFW_KEY_W))
    dolly(-k, true);
  if (keys->at(GLFW_KEY_S))
    dolly(k, true);
  if (keys->at(GLFW_KEY_A))
    truck(-k, 0.0f, true);
  if (keys->at(GLFW_KEY_D))
    truck(k, 0.0f, true);
}
