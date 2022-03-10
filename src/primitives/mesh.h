#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>

class Mesh
{
public:
	Mesh();

	void RenderMesh();

	virtual void createVertices() = 0;

	void translate(glm::vec3 const &translation);
	void rotate(glm::vec3 const &rotation);
	void scale(glm::vec3 const &scale);

	glm::f32 *getModelPtr();
	~Mesh();

protected:
	void CreateMesh();
	void ClearMesh();

	GLuint VAO, VBO, IBO;
	GLsizei indexCount;
	std::vector<GLfloat> vertices;
	std::vector<GLfloat> normals;
	std::vector<int> indices;
	glm::mat4 model;
};

#endif