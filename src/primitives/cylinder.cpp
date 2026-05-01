#include "cylinder.h"
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Cylinder::Cylinder(CylinderDimensions const &dimensions, CylinderParameters const &parameters)
		: Mesh{}, dimensions{dimensions}, parameters{parameters}
{
	createVertices();
	computeFaces();
	createMesh();
}

Cylinder::~Cylinder() = default;

void Cylinder::rebuildGeometry()
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

void Cylinder::createVertices()
{
	/** Three.js `CylinderGeometry` (r162): `radiusTop`, `radiusBottom`, height, radialSegments, heightSegments,
	 *  openEnded=false, thetaStart=0, thetaLength=2*pi */
	int const radialSegments = std::max(3, parameters.radialSegments);
	int const heightSegments = std::max(1, parameters.heightSegments);
	float const radiusTop = std::max(1e-4f, dimensions.radiusTop);
	float const radiusBottom = std::max(1e-4f, dimensions.radiusBottom);
	float const height = std::max(1e-4f, dimensions.height);
	float const thetaStart = 0.0f;
	float const thetaLength = static_cast<float>(2.0 * M_PI);

	float const halfHeight = height * 0.5f;
	float const slope = (radiusBottom - radiusTop) / height;

	std::vector<std::vector<int>> indexArray;

	vertices.clear();
	indices.clear();

	int index = 0;

	auto generateTorso = [&]()
	{
		for (int y = 0; y <= heightSegments; ++y)
		{
			std::vector<int> indexRow;
			float const v = static_cast<float>(y) / static_cast<float>(heightSegments);
			float const radius = v * (radiusBottom - radiusTop) + radiusTop;

			for (int x = 0; x <= radialSegments; ++x)
			{
				float const u = static_cast<float>(x) / static_cast<float>(radialSegments);
				float const theta = u * thetaLength + thetaStart;
				float const sinTheta = std::sin(theta);
				float const cosTheta = std::cos(theta);

				glm::vec3 vertex;
				vertex.x = radius * sinTheta;
				vertex.y = -v * height + halfHeight;
				vertex.z = radius * cosTheta;
				vertices.push_back(vertex);

				glm::vec3 normal(sinTheta, slope, cosTheta);
				normal = glm::normalize(normal);
				normals.push_back(normal);

				indexRow.push_back(index++);
			}
			indexArray.push_back(std::move(indexRow));
		}

		for (int x = 0; x < radialSegments; ++x)
		{
			for (int y = 0; y < heightSegments; ++y)
			{
				int const a = indexArray[y][x];
				int const b = indexArray[y + 1][x];
				int const c = indexArray[y + 1][x + 1];
				int const d = indexArray[y][x + 1];
				indices.push_back(static_cast<unsigned int>(a));
				indices.push_back(static_cast<unsigned int>(b));
				indices.push_back(static_cast<unsigned int>(d));
				indices.push_back(static_cast<unsigned int>(b));
				indices.push_back(static_cast<unsigned int>(c));
				indices.push_back(static_cast<unsigned int>(d));
			}
		}
	};

	auto generateCap = [&](bool top)
	{
		int const centerIndexStart = index;

		float const radius = top ? radiusTop : radiusBottom;
		float const sign = top ? 1.0f : -1.0f;

		for (int x = 1; x <= radialSegments; ++x)
		{
			vertices.push_back(glm::vec3(0.0f, halfHeight * sign, 0.0f));
			normals.push_back(glm::vec3(0.0f, sign, 0.0f));
			++index;
		}

		int const centerIndexEnd = index;

		for (int x = 0; x <= radialSegments; ++x)
		{
			float const u = static_cast<float>(x) / static_cast<float>(radialSegments);
			float const theta = u * thetaLength + thetaStart;
			float const cosTheta = std::cos(theta);
			float const sinTheta = std::sin(theta);

			glm::vec3 vertex;
			vertex.x = radius * sinTheta;
			vertex.y = halfHeight * sign;
			vertex.z = radius * cosTheta;
			vertices.push_back(vertex);

			normals.push_back(glm::vec3(0.0f, sign, 0.0f));
			++index;
		}

		for (int x = 0; x < radialSegments; ++x)
		{
			int const c = centerIndexStart + x;
			int const i = centerIndexEnd + x;
			if (top)
			{
				indices.push_back(static_cast<unsigned int>(i));
				indices.push_back(static_cast<unsigned int>(i + 1));
				indices.push_back(static_cast<unsigned int>(c));
			}
			else
			{
				indices.push_back(static_cast<unsigned int>(i + 1));
				indices.push_back(static_cast<unsigned int>(i));
				indices.push_back(static_cast<unsigned int>(c));
			}
		}
	};

	generateTorso();
	if (radiusTop > 0.0f)
		generateCap(true);
	if (radiusBottom > 0.0f)
		generateCap(false);

	/** Bake the old Y-up -> Z-up alignment into geometry so object-local Z is up at identity model. */
	for (auto &p : vertices)
		p = glm::vec3(p.x, -p.z, p.y);
	for (auto &n : normals)
		n = glm::normalize(glm::vec3(n.x, -n.z, n.y));
}
