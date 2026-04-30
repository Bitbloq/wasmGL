#ifndef TORUS_H
#define TORUS_H

#include "../core/mesh.h"

/** Standard torus around +Z: major = ring radius (hole to tube center), minor = tube radius. */
class TorusDimensions
{
public:
	TorusDimensions(GLfloat majorRadius = 0.6f, GLfloat minorRadius = 0.2f)
			: majorRadius{majorRadius}, minorRadius{minorRadius}
	{
	}
	GLfloat majorRadius;
	GLfloat minorRadius;
};

class TorusParameters
{
public:
	TorusParameters(int radialSegments = 16, int tubularSegments = 32)
			: radialSegments{radialSegments}, tubularSegments{tubularSegments}
	{
	}
	int radialSegments;
	int tubularSegments;
};

class Torus : public Mesh
{
public:
	Torus(TorusDimensions const &dimensions = TorusDimensions{},
			TorusParameters const &parameters = TorusParameters{});
	~Torus();

	TorusDimensions getDimensions() const { return dimensions; }
	void setDimensions(TorusDimensions const &d) { dimensions = d; }
	TorusParameters getParameters() const { return parameters; }
	void setParameters(TorusParameters const &p) { parameters = p; }
	void rebuildGeometry();

private:
	void createVertices();
	TorusDimensions dimensions;
	TorusParameters parameters;
};

#endif
