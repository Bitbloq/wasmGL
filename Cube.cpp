#include "Cube.h"
#include <iostream>
#include <math.h>

Cube::Cube(CubeDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
  createVertices();
  CreateMesh();
}

Cube::~Cube()
{
}

void Cube::createVertices()
{

  indices = {
      0, 1, 2,
      0, 2, 3,
      0, 3, 4,
      0, 4, 5,
      0, 5, 1,
      1, 5, 6,
      1, 6, 2,
      2, 6, 7,
      2, 7, 3,
      3, 7, 4,
      4, 7, 5,
      5, 7, 6};

  vertices = {
      -0.5f, -0.5f, -0.5f,
      0.5f, -0.5f, -0.5f,
      0.5f, 0.5f, -0.5f,
      -0.5f, 0.5f, -0.5f,
      -0.5f, -0.5f, 0.5f,
      0.5f, -0.5f, 0.5f,
      0.5f, 0.5f, 0.5f,
      -0.5f, 0.5f, 0.5f};
}
