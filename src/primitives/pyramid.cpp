#include "pyramid.h"
#include <iostream>
#include <math.h>

Pyramid::Pyramid(PyramidDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
    createVertices();
    computeFaces();
    createMesh();
}

Pyramid::Pyramid(Pyramid const &pyramid) : Mesh{}, dimensions{pyramid.dimensions}
{
    createVertices();
    createMesh();
}

Pyramid::~Pyramid()
{
}

void Pyramid::createVertices()
{
    const auto dxsqrt3 = std::sqrt(3.0f) * dimensions.side;

    indices = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 1,
        1, 3, 2};

    vertices = {
        {-0.5f * dimensions.side, -dxsqrt3 / 6.0f, 0.0f},
        {0.5f * dimensions.side,
         -dxsqrt3 / 6.0f,
         0.0f},
        {0.0f,
         dxsqrt3 / 3.0f,
         0.0f},
        {0.0f,
         0.0f,
         dimensions.height}};
}
