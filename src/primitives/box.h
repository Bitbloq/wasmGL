#ifndef BOX_H
#define BOX_H

#include "../core/mesh.h"

struct BoxDimensions
{
  GLfloat width;
  GLfloat height;
  GLfloat depth;
};

class Box : public Mesh
{
public:
  Box(BoxDimensions dimensions = BoxDimensions{1.0f});
  ~Box();

  inline BoxDimensions getDimensions() const { return dimensions; };
  inline void setDimensions(BoxDimensions const &dimensions) { this->dimensions = dimensions; };

private:
  void createVertices();
  BoxDimensions dimensions;
};

#endif