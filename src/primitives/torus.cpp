#include "torus.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/** Three.js `TorusGeometry` (r162): radius, tube, radialSegments, tubularSegments, arc=2π */

Torus::Torus(TorusDimensions const &dimensions, TorusParameters const &parameters)
		: Mesh{}, dimensions{dimensions}, parameters{parameters}
{
	createVertices();
	computeFaces();
	createMesh();
}

Torus::~Torus() = default;

void Torus::rebuildGeometry()
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
	computeThreeBSP();
}

void Torus::createVertices()
{
	int const radialSegments = std::max(3, parameters.radialSegments);
	int const tubularSegments = std::max(3, parameters.tubularSegments);
	float const radius = std::max(1e-4f, dimensions.majorRadius);
	float const tube = std::max(1e-4f, dimensions.minorRadius);
	float const arc = static_cast<float>(2.0 * M_PI);

	vertices.clear();
	normals.clear();
	indices.clear();

	glm::vec3 center(0.0f);

	for (int j = 0; j <= radialSegments; ++j)
	{
		for (int i = 0; i <= tubularSegments; ++i)
		{
			float const u = static_cast<float>(i) / static_cast<float>(tubularSegments) * arc;
			float const v = static_cast<float>(j) / static_cast<float>(radialSegments) * static_cast<float>(2.0 * M_PI);

			glm::vec3 vertex;
			vertex.x = (radius + tube * std::cos(v)) * std::cos(u);
			vertex.y = (radius + tube * std::cos(v)) * std::sin(u);
			vertex.z = tube * std::sin(v);
			vertices.push_back(vertex);

			center.x = radius * std::cos(u);
			center.y = radius * std::sin(u);
			center.z = 0.0f;
			glm::vec3 normal = vertex - center;
			float const nl = glm::length(normal);
			normal = nl > 1e-20f ? normal / nl : glm::vec3(0.0f, 0.0f, 1.0f);
			normals.push_back(normal);
		}
	}

	for (int j = 1; j <= radialSegments; ++j)
	{
		for (int i = 1; i <= tubularSegments; ++i)
		{
			int const a = (tubularSegments + 1) * j + i - 1;
			int const b = (tubularSegments + 1) * (j - 1) + i - 1;
			int const c = (tubularSegments + 1) * (j - 1) + i;
			int const d = (tubularSegments + 1) * j + i;
			indices.push_back(static_cast<unsigned int>(a));
			indices.push_back(static_cast<unsigned int>(b));
			indices.push_back(static_cast<unsigned int>(d));
			indices.push_back(static_cast<unsigned int>(b));
			indices.push_back(static_cast<unsigned int>(c));
			indices.push_back(static_cast<unsigned int>(d));
		}
	}
}
