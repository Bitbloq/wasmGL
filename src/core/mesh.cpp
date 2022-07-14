#include "mesh.h"
#include <iostream>
#include "../threecsg/threebsp.h"

Mesh::Mesh() : VAO{0}, VBO{0}, IBO{0}, indexCount{0}, model{glm::mat4(1.0f)}
{
	colorsNeedUpdate = false;
	facesNeedUpdate = false;
	verticesNeedUpdate = false;
	// faceVertexUvsNeedUpdate = false;
}

void Mesh::computeThreeBSP()
{
	threeBSP = make_shared<ThreeBSP>(ThreeBSP(shared_from_this()));
}

shared_ptr<Mesh> Mesh::subtract(shared_ptr<Mesh> const &other)
{
	auto csg = threeBSP->subtract(other->getThreeBSP());
	std::shared_ptr<CSGMesh> csgObj = csg->toMesh();
	return csgObj;
}

void Mesh::translate(glm::vec3 const &translation)
{
	model = glm::translate(model, translation);
}

void Mesh::rotate(glm::vec3 const &rotation)
{
	model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

void Mesh::scale(glm::vec3 const &scale)
{
	model = glm::scale(model, scale);
}

glm::f32 *Mesh::getModelPtr()
{
	return glm::value_ptr(model);
}

void Mesh::computeFaces()
{
	for (size_t i{0}; i < indices.size(); i += 3)
	{
		glm::vec3 faceNormal = glm::cross(vertices[indices[i + 1]] - vertices[indices[i]], vertices[indices[i + 2]] - vertices[indices[i]]);
		faceNormal = glm::normalize(faceNormal);

		array<glm::vec3, 3> vertexNormals = {
				faceNormal,
				faceNormal,
				faceNormal};

		faces.push_back(make_shared<Face3>(indices[i], indices[i + 1], indices[i + 2], faceNormal, vertexNormals));
	}
}

void Mesh::createMesh()
{
	indexCount = indices.size();

	glGenVertexArrays(1, &VAO); // Generate 1 vertex array object
	glBindVertexArray(VAO);			// Bind the VAO

	glGenBuffers(1, &IBO);																																												 // Generate 1 buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);																																		 // Bind the IBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices.at(0)) * indices.size(), indices.data(), GL_STATIC_DRAW); // Copy the index data to the buffer object

	glGenBuffers(1, &VBO); // Generate 1 buffer object
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// convert vertices to lineal vector
	std::vector<GLfloat> vert;
	for (auto v : vertices)
	{
		vert.push_back(v.x);
		vert.push_back(v.y);
		vert.push_back(v.z);
	}
	// Bind the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert.at(0)) * vert.size(), vert.data(), GL_STATIC_DRAW); // Copy the vertex data to the buffer object

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Vertex position
	glEnableVertexAttribArray(0);													 // Enable the vertex position attribute

	glBindBuffer(GL_ARRAY_BUFFER, 0);					// Unbind the VBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind the IBO

	glBindVertexArray(0); // Unbind the VAO
}

void Mesh::RenderMesh()
{

	glBindVertexArray(VAO);											// Bind the VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO); // Bind the IBO
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::ClearMesh()
{
	if (IBO != 0)
	{
		glDeleteBuffers(1, &IBO);
		IBO = 0;
	}

	if (VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}

	if (VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}

	indexCount = 0;
}

Mesh::~Mesh()
{
	ClearMesh();
}