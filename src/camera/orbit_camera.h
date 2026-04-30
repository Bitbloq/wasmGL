#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include <array>
#include <memory>
#include "wasmgl_gl.h"
#include "glm/glm.hpp"

/** Three.js–style orbit: spherical coords around target, smoothed with damping. */
class OrbitCamera
{
public:
  /**
   * Default eye: above the default BaseGrid (±10 in X/Y), diagonal in XY, tilted from +Z so the
   * full grid is in frame at ~48° vertical FOV (adjust radius if grid size changes).
   */
  explicit OrbitCamera(glm::vec3 const &worldPosition = glm::vec3(13.5f, 13.5f, 15.5f),
                       glm::vec3 const &target = glm::vec3(0.0f));

  glm::mat4 calculateViewMatrix() const;
  glm::vec3 getPosition() const { return position_; }

  void setFovDegrees(GLfloat fov) { fovDegrees_ = fov; }

  void keyControl(std::shared_ptr<std::array<bool, 1024>> keys, GLfloat deltaTime);

  /** Pointer deltas from GLFW (xChange = dx right, yChange = dy up — match Window). */
  void applyRotatePixels(GLfloat xChange, GLfloat yChange, GLfloat canvasW, GLfloat canvasH);
  void applyPanPixels(GLfloat xChange, GLfloat yChange, GLfloat canvasW, GLfloat canvasH);
  /** GLFW scroll y offset (positive / negative → dolly out / in like wheel delta). */
  void applyWheel(GLfloat yScrollOffset);

  void setDragging(bool dragging) { dragging_ = dragging; }

  void nudgeViewYawDegrees(GLfloat deltaYawDeg);
  void nudgeViewPitchDegrees(GLfloat deltaPitchDeg);
  void nudgePositionView(GLfloat alongFront, GLfloat alongRight, GLfloat alongUp);

  void zoomInButton();
  void zoomOutButton();

  bool update(GLfloat deltaTime);

private:
  struct Spherical
  {
    float radius{1.0f};
    float phi{static_cast<float>(M_PI) * 0.5f};
    float theta{0.0f};

    void setFromOffset(glm::vec3 const &v);
    void makeSafe();
    glm::vec3 cartesian() const;
  };

  void rotate(float rotTheta, float rotPhi, bool enableTransition);
  void rotateTo(float theta, float phi, bool enableTransition);
  void dolly(float distance, bool enableTransition);
  void dollyTo(float distance, bool enableTransition);
  void truck(float x, float y, bool enableTransition);

  glm::vec3 target_{};
  glm::vec3 targetEnd_{};

  Spherical spherical_{};
  Spherical sphericalEnd_{};

  glm::vec3 position_{};

  GLfloat dampingFactor_{0.05f};
  GLfloat draggingDampingFactor_{0.25f};
  GLfloat dollySpeed_{1.0f};
  GLfloat truckSpeed_{2.0f};
  GLfloat minDistance_{0.0f};
  GLfloat maxDistance_{1.0e9f};
  GLfloat minPolarAngle_{0.0f};
  GLfloat maxPolarAngle_{static_cast<float>(M_PI)};
  GLfloat minAzimuthAngle_{-1.0e9f};
  GLfloat maxAzimuthAngle_{1.0e9f};

  GLfloat fovDegrees_{48.0f};
  GLfloat movementSpeed_{5.0f};

  bool dragging_{false};
  bool needsUpdate_{true};
};

#endif
