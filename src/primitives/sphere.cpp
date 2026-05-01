#include "sphere.h"
#include <cmath>
#include <iostream>
#include <vector>

/** Three.js `SphereGeometry` (r162) - same vertex, normal, and index order. */

Sphere::Sphere(SphereDimensions const &dimensions, SphereParameters const &parameters) : Mesh{}, dimensions{dimensions}, parameters{parameters}
{
	createVertices();
	computeFaces();
	createMesh();
}

Sphere::~Sphere()
{
}

void Sphere::rebuildGeometry()
{
	ClearMesh();
	vertices.clear();
	normals.clear();
	faces.clear();
	indices.clear();
	createVertices();
	computeFaces();
	createMesh();
	threeBSPDone = false;
}

void Sphere::createVertices()
{
	widthSegments = std::max(3, parameters.widthSegments);
	heightSegments = std::max(2, parameters.heightSegments);

	float const radius = std::max(1e-4f, dimensions.radius);
	float const phiStart = parameters.phiStart;
	float const phiLength = parameters.phiLength;
	float const thetaStart = parameters.thetaStart;
	float const thetaLength = parameters.thetaLength;

	float const thetaEnd = std::min(thetaStart + thetaLength, static_cast<float>(M_PI));

	vertices.clear();
	normals.clear();
	indices.clear();

	std::vector<std::vector<int>> grid;
	int index = 0;

	for (size_t iy = 0; iy <= static_cast<size_t>(heightSegments); ++iy)
	{
		std::vector<int> verticesRow;
		float const v = static_cast<float>(iy) / static_cast<float>(heightSegments);

		float uOffset = 0.0f;
		if (iy == 0 && thetaStart == 0.0f)
			uOffset = 0.5f / static_cast<float>(widthSegments);
		else if (static_cast<int>(iy) == heightSegments && thetaEnd >= static_cast<float>(M_PI))
			uOffset = -0.5f / static_cast<float>(widthSegments);

		for (size_t ix = 0; ix <= static_cast<size_t>(widthSegments); ++ix)
		{
			float const u = static_cast<float>(ix) / static_cast<float>(widthSegments);

			glm::vec3 vertex;
			vertex.x = -radius * std::cos(phiStart + u * phiLength) * std::sin(thetaStart + v * thetaLength);
			vertex.y = radius * std::cos(thetaStart + v * thetaLength);
			vertex.z = radius * std::sin(phiStart + u * phiLength) * std::sin(thetaStart + v * thetaLength);
			vertices.push_back(vertex);

			glm::vec3 const normal = glm::normalize(vertex);
			normals.push_back(normal);

			verticesRow.push_back(index++);
		}
		grid.push_back(std::move(verticesRow));
	}

	for (size_t iy = 0; iy < static_cast<size_t>(heightSegments); ++iy)
	{
		for (size_t ix = 0; ix < static_cast<size_t>(widthSegments); ++ix)
		{
			int const a = grid[iy][ix + 1];
			int const b = grid[iy][ix];
			int const c = grid[iy + 1][ix];
			int const d = grid[iy + 1][ix + 1];

			if (iy != 0 || thetaStart > 0.0f)
			{
				indices.push_back(static_cast<unsigned int>(a));
				indices.push_back(static_cast<unsigned int>(b));
				indices.push_back(static_cast<unsigned int>(d));
			}
			if (static_cast<int>(iy) != heightSegments - 1 || thetaEnd < static_cast<float>(M_PI))
			{
				indices.push_back(static_cast<unsigned int>(b));
				indices.push_back(static_cast<unsigned int>(c));
				indices.push_back(static_cast<unsigned int>(d));
			}
		}
	}
}
