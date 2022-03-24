#include "pyramid.h"
#include <iostream>
#include <math.h>

Pyramid::Pyramid(PyramidDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
    createVertices();
    computeFaces();
    CreateMesh();
}

Pyramid::Pyramid(Pyramid const &pyramid) : Mesh{}, dimensions{pyramid.dimensions}
{
    createVertices();
    CreateMesh();
}

Pyramid::~Pyramid()
{
}

void Pyramid::createVertices()
{
    const auto sqrt3 = std::sqrt(3.0f);

    indices = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 1,
        1, 3, 2};

    vertices = {
        {-0.5f, -sqrt3 / 6.0f, 0.0f},
        {0.5f,
         -sqrt3 / 6.0f,
         0.0f},
        {0.0f,
         sqrt3 / 3.0f,
         0.0f},
        {0.0f,
         0.0f,
         1.0f}};
}
