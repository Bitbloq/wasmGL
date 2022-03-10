#include "Mesh.h"
#include <iostream>

Mesh::Mesh() : VAO{0}, VBO{0}, IBO{0}, indexCount{0}, model{glm::mat4(1.0f)}
{
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

void Mesh::CreateMesh()
{
	indexCount = indices.size();

	glGenVertexArrays(1, &VAO); // Generate 1 vertex array object
	glBindVertexArray(VAO);			// Bind the VAO

	glGenBuffers(1, &IBO);																																												 // Generate 1 buffer object
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);																																		 // Bind the IBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices.at(0)) * indices.size(), indices.data(), GL_STATIC_DRAW); // Copy the index data to the buffer object

	glGenBuffers(1, &VBO);																																										// Generate 1 buffer object
	glBindBuffer(GL_ARRAY_BUFFER, VBO);																																				// Bind the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices.at(0)) * vertices.size(), vertices.data(), GL_STATIC_DRAW); // Copy the vertex data to the buffer object

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
