#include "box.h"
#include <iostream>
#include <math.h>
#include <vector>
#include "../threecsg/threebsp.h"

namespace
{
	int axisOf(char c)
	{
		return c == 'x' ? 0 : (c == 'y' ? 1 : 2);
	}
	void setComp(glm::vec3 &p, int a, float val)
	{
		if (a == 0)
			p.x = val;
		else if (a == 1)
			p.y = val;
		else
			p.z = val;
	}

	/** Mirrors `buildPlane` in Three.js `src/geometries/BoxGeometry.js` (r162). */
	void buildPlane(char u, char v, char w, float udir, float vdir,
									float width, float height, float depth,
									int gridX, int gridY,
									std::vector<glm::vec3> &vertices, std::vector<int> &indices)
	{
		float const segmentWidth = width / static_cast<float>(gridX);
		float const segmentHeight = height / static_cast<float>(gridY);
		float const widthHalf = width * 0.5f;
		float const heightHalf = height * 0.5f;
		float const depthHalf = depth * 0.5f;
		int const gridX1 = gridX + 1;
		int const gridY1 = gridY + 1;

		int const U = axisOf(u);
		int const V = axisOf(v);
		int const W = axisOf(w);

		int const numberOfVertices = static_cast<int>(vertices.size());

		for (int iy = 0; iy < gridY1; ++iy)
		{
			float const y = iy * segmentHeight - heightHalf;
			for (int ix = 0; ix < gridX1; ++ix)
			{
				float const x = ix * segmentWidth - widthHalf;
				glm::vec3 vector(0.0f);
				setComp(vector, U, x * udir);
				setComp(vector, V, y * vdir);
				setComp(vector, W, depthHalf);
				vertices.push_back(vector);
			}
		}

		for (int iy = 0; iy < gridY; ++iy)
		{
			for (int ix = 0; ix < gridX; ++ix)
			{
				int const a = numberOfVertices + ix + gridX1 * iy;
				int const b = numberOfVertices + ix + gridX1 * (iy + 1);
				int const c = numberOfVertices + (ix + 1) + gridX1 * (iy + 1);
				int const d = numberOfVertices + (ix + 1) + gridX1 * iy;
				indices.push_back(a);
				indices.push_back(b);
				indices.push_back(d);
				indices.push_back(b);
				indices.push_back(c);
				indices.push_back(d);
			}
		}
	}
}

Box::Box(BoxDimensions dimensions) : Mesh{}, dimensions{dimensions}
{
	createVertices();
	computeFaces();
	createMesh();
}

Box::~Box()
{
}

void Box::rebuildGeometry()
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

void Box::createVertices()
{
	float const w = std::max(1e-4f, dimensions.width);
	float const h = std::max(1e-4f, dimensions.height);
	float const d = std::max(1e-4f, dimensions.depth);
	/** Default segment counts == `new THREE.BoxGeometry(w,h,d)` (1,1,1). */
	int const widthSegments = 1;
	int const heightSegments = 1;
	int const depthSegments = 1;

	vertices.clear();
	indices.clear();

	/** Same face order and `buildPlane` args as Three.js `BoxGeometry`. */
	buildPlane('z', 'y', 'x', -1.0f, -1.0f, d, h, w, depthSegments, heightSegments, vertices, indices);
	buildPlane('z', 'y', 'x', 1.0f, -1.0f, d, h, -w, depthSegments, heightSegments, vertices, indices);
	buildPlane('x', 'z', 'y', 1.0f, 1.0f, w, d, h, widthSegments, depthSegments, vertices, indices);
	buildPlane('x', 'z', 'y', 1.0f, -1.0f, w, d, -h, widthSegments, depthSegments, vertices, indices);
	buildPlane('x', 'y', 'z', 1.0f, -1.0f, w, h, d, widthSegments, heightSegments, vertices, indices);
	buildPlane('x', 'y', 'z', -1.0f, -1.0f, w, h, -d, widthSegments, heightSegments, vertices, indices);
}
