#include "sphere.h"
#include <cmath>
#include <iostream>

Sphere::Sphere(SphereDimensions const &dimensions, SphereParameters const &parameters) : Mesh{}, dimensions{dimensions}, parameters{parameters}
{
  createVertices();
  computeFaces();
  createMesh();
}

Sphere::~Sphere()
{
}

void Sphere::createVertices()
{
  auto radius = dimensions.radius;
  auto heightSegments = parameters.heightSegments;
  auto widthSegments = parameters.widthSegments;
  auto phiStart = parameters.phiStart;
  auto phiLength = parameters.phiLength;
  auto thetaStart = parameters.thetaStart;
  auto thetaLength = parameters.thetaLength;

  int index = 0;
  std::vector<std::vector<int>> grid;

  widthSegments = std::max(3, widthSegments);
  heightSegments = std::max(2, heightSegments);

  auto thetaEnd = std::min(thetaStart + thetaLength, float(M_PI));

  for (size_t iy{0}; iy <= heightSegments; iy++)
  {
    std::vector<int> verticesRow;
    auto v = iy / (float)heightSegments;
    auto uOffset = 0;
    if (iy == 0 && thetaStart == 0)
    {
      uOffset = 0.5 / widthSegments;
    }
    else if (iy == heightSegments && thetaEnd >= M_PI)
    {
      uOffset = -0.5 / widthSegments;
    }

    for (size_t ix{0}; ix <= widthSegments; ix++)
    {
      auto u = ix / (float)widthSegments;
      auto vertex = glm::vec3();
      vertex.x = -radius * cos(phiStart + u * phiLength) * sin(thetaStart + v * thetaLength);
      vertex.y = radius * cos(thetaStart + v * thetaLength);
      vertex.z = radius * sin(phiStart + u * phiLength) * sin(thetaStart + v * thetaLength);
      this->vertices.push_back(vertex);

      // normal
      auto normal = glm::normalize(glm::vec3(vertex));
      this->normals.push_back(normal);

      // uv
      // auto uv = glm::vec2(u + uOffset, 1.0f - v);
      // this->uvs.push_back(uv);

      verticesRow.push_back(index++);
    }
    grid.push_back(verticesRow);
  }

  // indices

  for (size_t iy{0}; iy < heightSegments; iy++)
  {
    for (size_t ix{0}; ix < widthSegments; ix++)
    {
      auto a = grid.at(iy).at(ix + 1);
      auto b = grid.at(iy).at(ix);
      auto c = grid.at(iy + 1).at(ix);
      auto d = grid.at(iy + 1).at(ix + 1);

      if (iy != 0 || thetaStart > 0)
      {
        this->indices.push_back(a);
        this->indices.push_back(b);
        this->indices.push_back(d);
      }
      if (iy != heightSegments - 1 || thetaEnd < M_PI)
      {
        this->indices.push_back(b);
        this->indices.push_back(c);
        this->indices.push_back(d);
      }
    }
  }
}
