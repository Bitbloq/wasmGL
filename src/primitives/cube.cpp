#include "cube.h"
#include <iostream>
#include <math.h>

Cube::Cube(CubeDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
  createVertices();
  computeFaces();
  CreateMesh();
}

Cube::~Cube()
{
}

void Cube::createVertices()
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
      {0.5, 0.5, 0.5},
      {0.5, 0.5, -0.5},
      {0.5, -0.5, 0.5},
      {0.5, -0.5, -0.5},
      {-0.5, 0.5, -0.5},
      {-0.5, 0.5, 0.5},
      {-0.5, -0.5, -0.5},
      {-0.5, -0.5, 0.5}};
}
