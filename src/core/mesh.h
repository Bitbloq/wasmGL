#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <string>
#include "./color.h"
#include "./face3.h"

using namespace std;

class Mesh
{
	friend class ThreeBSP;

public:
	Mesh();

	void RenderMesh();

	virtual void createVertices() = 0;

	void translate(glm::vec3 const &translation);
	void rotate(glm::vec3 const &rotation);
	void scale(glm::vec3 const &scale);

	glm::mat4 getModelMatrix() const { return model; }

	glm::f32 *getModelPtr();
	~Mesh();

protected:
	void CreateMesh();
	void ClearMesh();
	void computeFaces();

	GLuint VAO,
			VBO, IBO;
	GLsizei indexCount;
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
	std::vector<glm::vec2> uvs;

	glm::mat4 model;

	int id; // unique id for this instance
	string uuid;
};

#endif