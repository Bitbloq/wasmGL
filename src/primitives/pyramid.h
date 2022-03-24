#ifndef PYRAMID_H
#define PYRAMID_H

#include "../core/mesh.h"

struct PyramidDimensions
{
  GLfloat side, height;
};

class Pyramid : public Mesh
{
public:
  Pyramid(PyramidDimensions dimensions = PyramidDimensions{1.0f, 1.0f});
  Pyramid(Pyramid const &pyramid);
  ~Pyramid();

  inline PyramidDimensions getDimensions() const { return dimensions; };
  inline void setDimensions(PyramidDimensions const &dimensions) { this->dimensions = dimensions; };

private:
  void createVertices();
  PyramidDimensions dimensions;
};

#endif