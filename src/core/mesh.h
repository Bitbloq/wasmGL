#ifndef MESH_H
#define MESH_H

#include "wasmgl_gl.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <iostream>

#include "./color.h"
#include "./face3.h"

using namespace std;

class ThreeBSP;

class Mesh : public std::enable_shared_from_this<Mesh>
{
	friend class ThreeBSP;

public:
	Mesh();

	void RenderMesh();
	/** Black/dark line overlay so edges read clearly (WebGL2-safe; no glPolygonMode). */
	void RenderWireframe();

	virtual void createVertices() = 0;

	void translate(glm::vec3 const &translation);
	void rotate(glm::vec3 const &rotation);
	void scale(glm::vec3 const &scale);

	shared_ptr<glm::mat4> getModelMatrix() const { return model; }
	bool getLocalBounds(glm::vec3 &minOut, glm::vec3 &maxOut) const;

	glm::f32 *getModelPtr();

	void setSolidColor(glm::vec3 const &rgb) { solidColor = rgb; }
	glm::vec3 getSolidColor() const { return solidColor; }

	void setThreeBSP(shared_ptr<ThreeBSP> const &bsp)
	{
		threeBSP = bsp;
	}
	shared_ptr<ThreeBSP> getThreeBSP() const { return threeBSP; }

	shared_ptr<Mesh> subtract(shared_ptr<Mesh> const &other);
	shared_ptr<Mesh> add(shared_ptr<Mesh> const &other);
	shared_ptr<Mesh> intersect(shared_ptr<Mesh> const &other);

	std::function<void()> computeThreeBSPLambda;
	void computeThreeBSP();
	std::atomic<bool> threeBSPDone; // Use an atomic flag.
	void setModel(shared_ptr<glm::mat4> const &model) { this->model = model; }

	~Mesh();

protected:
	void createMesh();
	void ClearMesh();
	void computeFaces();
	/** Fills `normals` from `faces` if needed (box / CSG). Sphere already sets normals. */
	void ensureVertexNormals();
	void buildWireframeEdgeBuffer();

	shared_ptr<ThreeBSP> threeBSP;

	GLuint VAO,
			VBO, IBO;
	GLuint edgeIBO{0};
	GLsizei indexCount;
	GLsizei edgeIndexCount{0};
	// std::vector<GLfloat> vertices;

	/**
	 * The array of vertices hold every position of points of the model.
	 */
	std::vector<glm::vec3> vertices;
	bool verticesNeedUpdate;

	/**
	 * Array of vertex colors, matching number and order of vertices.
	 * Used in ParticleSystem, Line and Ribbon.
	 * Meshes use per-face-use-of-vertex colors embedded directly in faces.
	 */
	std::vector<Color> colors;
	bool colorsNeedUpdate;

	/**
	 * Array of triangles or/and quads.
	 * The array of faces describe how each vertex in the model is connected with each other.
	 * To signal an update in this array, Geometry.elementsNeedUpdate needs to be set to true.
	 */
	std::vector<shared_ptr<Face3>> faces;
	bool facesNeedUpdate;

	/**
	 * Array of face UV layers.
	 * Each UV layer is an array of UV matching order and number of vertices in faces.
	 * To signal an update in this array, Geometry.uvsNeedUpdate needs to be set to true.
	 */
	// std::vector<std::vector<std::vector<shared_ptr<glm::vec2>>>> faceVertexUvs;
	// bool faceVertexUvsNeedUpdate;

	std::vector<glm::vec3> normals;
	std::vector<int> indices;
	// std::vector<glm::vec2> uvs;

	shared_ptr<glm::mat4> model;

	/** Albedo for Phong shading (linear RGB, 0-1). */
	glm::vec3 solidColor{0.45f, 0.55f, 0.85f};

	int id; // unique id for this instance
	string uuid;
};

#endif
