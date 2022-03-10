#ifndef CUBE_H
#define CUBE_H

#include "mesh.h"

struct CubeDimensions
{
  GLfloat side;
};

class Cube : public Mesh
{
public:
  Cube(CubeDimensions dimensions = CubeDimensions{1.0f});
  ~Cube();

  inline CubeDimensions getDimensions() const { return dimensions; };
  inline void setDimensions(CubeDimensions const &dimensions) { this->dimensions = dimensions; };

private:
  void createVertices();
  CubeDimensions dimensions;
};

#endif