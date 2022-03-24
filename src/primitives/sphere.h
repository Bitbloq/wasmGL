#ifndef SPHERE_H
#define SPHERE_H

#include "../core/mesh.h"

class SphereDimensions
{
public:
  SphereDimensions(GLfloat radius = 1.0f) : radius{radius} {}
  GLfloat radius;
};

class SphereParameters
{
public:
  SphereParameters(int widthSegments = 12,
                   int heightSegments = 16,
                   float phiStart = 0.0f,
                   float phiLength = 2 * M_PI,
                   float thetaStart = 0.0f,
                   float thetaLength = M_PI)
      : widthSegments{widthSegments},
        heightSegments{heightSegments},
        phiStart{phiStart},
        phiLength{phiLength},
        thetaStart{thetaStart},
        thetaLength{thetaLength} {}
  int widthSegments;
  int heightSegments;
  float phiStart;
  float phiLength;
  float thetaStart;
  float thetaLength;
};

class Sphere : public Mesh
{
public:
  Sphere(SphereDimensions const &dimensions = SphereDimensions{1.0f}, SphereParameters const &parameters = SphereParameters{});
  Sphere(Sphere const &Sphere);
  ~Sphere();

  inline SphereDimensions getDimensions() const { return dimensions; };
  inline void setDimensions(SphereDimensions const &dimensions) { this->dimensions = dimensions; };

private:
  void createVertices();
  SphereDimensions dimensions;
  SphereParameters parameters;
  int widthSegments;
  int heightSegments;
};

#endif