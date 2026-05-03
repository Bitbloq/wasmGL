#ifndef CYLINDER_H
#define CYLINDER_H

#include "../core/mesh.h"

/** Full height along object-local Z after geometry creation. Equal radii = right cylinder. */
class CylinderDimensions
{
public:
	CylinderDimensions(GLfloat radiusBottom, GLfloat radiusTop, GLfloat height)
			: radiusBottom{radiusBottom}, radiusTop{radiusTop}, height{height}
	{
	}
	GLfloat radiusBottom;
	GLfloat radiusTop;
	GLfloat height;
};

class CylinderParameters
{
public:
	CylinderParameters(int radialSegments = 16, int heightSegments = 1)
			: radialSegments{radialSegments}, heightSegments{heightSegments}
	{
	}
	int radialSegments;
	int heightSegments;
};

class Cylinder : public Mesh
{
public:
	Cylinder(CylinderDimensions const &dimensions = CylinderDimensions{0.5f, 0.5f, 1.0f},
			CylinderParameters const &parameters = CylinderParameters{});
	~Cylinder();

	CylinderDimensions getDimensions() const { return dimensions; }
	void setDimensions(CylinderDimensions const &d) { dimensions = d; }
	CylinderParameters getParameters() const { return parameters; }
	void setParameters(CylinderParameters const &p) { parameters = p; }
	void rebuildGeometry();

private:
	void createVertices();
	CylinderDimensions dimensions;
	CylinderParameters parameters;
};

#endif
