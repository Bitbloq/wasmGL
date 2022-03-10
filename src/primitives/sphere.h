#ifndef SPHERE_H
#define SPHERE_H

#include "mesh.h"

struct SphereDimensions
{
  GLfloat radius;
};

class Sphere : public Mesh
{
public:
  Sphere(SphereDimensions dimensions = SphereDimensions{1.0f});
  Sphere(Sphere const &Sphere);
  ~Sphere();

  inline SphereDimensions getDimensions() const { return dimensions; };
  inline void setDimensions(SphereDimensions const &dimensions) { this->dimensions = dimensions; };

private:
  void createVertices();
  SphereDimensions dimensions;
  float radius;
  int sectorCount;
  int stackCount;
};

#endif