#include "box.h"
#include <iostream>
#include <math.h>

Box::Box(BoxDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
  createVertices();
  computeFaces();
  CreateMesh();
}

Box::~Box()
{
}

void Box::createVertices()
{

  indices = {
      0, 2, 1,
      2, 3, 1,
      4, 6, 5,
      6, 7, 5,
      4, 5, 1,
      5, 0, 1,
      7, 6, 2,
      6, 3, 2,
      5, 7, 0,
      7, 2, 0,
      1, 3, 4,
      3, 6, 4};

  vertices = {
      {dimensions.width / 2, dimensions.height / 2, dimensions.depth / 2},
      {dimensions.width / 2, dimensions.height / 2, -dimensions.depth / 2},
      {dimensions.width / 2, -dimensions.height / 2, dimensions.depth / 2},
      {dimensions.width / 2, -dimensions.height / 2, -dimensions.depth / 2},
      {-dimensions.width / 2, dimensions.height / 2, -dimensions.depth / 2},
      {-dimensions.width / 2, dimensions.height / 2, dimensions.depth / 2},
      {-dimensions.width / 2, -dimensions.height / 2, -dimensions.depth / 2},
      {-dimensions.width / 2, -dimensions.height / 2, dimensions.depth / 2}};
}
