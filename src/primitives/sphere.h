#ifndef SPHERE_H
#define SPHERE_H

#include "../core/mesh.h"

struct SphereDimensions
{
  GLfloat radius;
};

struct SphereParameters
{
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
  Sphere(SphereDimensions const &dimensions = {1.0f}, SphereParameters const &parameters = {6, 8, 0.0f, 2 * M_PI, 0.0f, M_PI});
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