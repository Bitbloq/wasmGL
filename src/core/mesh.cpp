#include "mesh.h"
#include <iostream>
#include <set>
#include <algorithm>
#include "../threecsg/threebsp.h"

Mesh::Mesh() : VAO{0}, VBO{0}, IBO{0}, indexCount{0}, model{make_shared<glm::mat4>(1.0f)}, threeBSPDone{false}
{
	colorsNeedUpdate = false;
	facesNeedUpdate = false;
	verticesNeedUpdate = false;
	computeThreeBSPLambda = [&]()
	{
		threeBSP = make_shared<ThreeBSP>(ThreeBSP(shared_from_this()));
	};
}

void Mesh::ensureVertexNormals()
{
	if (vertices.empty())
		return;
	if (normals.size() == vertices.size())
	{
		for (auto &n : normals)
		{
			if (glm::length(n) > 1e-6f)
				n = glm::normalize(n);
			else
				n = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		return;
	}
	normals.assign(vertices.size(), glm::vec3(0.0f));
	for (auto const &f : faces)
	{
		glm::vec3 fn = glm::length(f->normal) > 1e-6f ? glm::normalize(f->normal) : glm::vec3(0.0f, 1.0f, 0.0f);
		normals[f->a] += fn;
		normals[f->b] += fn;
		normals[f->c] += fn;
	}
	for (auto &n : normals)
		n = glm::length(n) > 1e-6f ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
}

void Mesh::buildWireframeEdgeBuffer()
{
	if (indices.size() < 3)
	{
		edgeIndexCount = 0;
		return;
	}

	std::set<std::pair<int, int>> edgeSet;
	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		int tri[3] = {indices[i], indices[i + 1], indices[i + 2]};
		for (int e = 0; e < 3; ++e)
		{
			int a = tri[e];
			int b = tri[(e + 1) % 3];
			if (a > b)
				std::swap(a, b);
			edgeSet.insert({a, b});
		}
	}

	std::vector<unsigned int> lineIndices;
	lineIndices.reserve(edgeSet.size() * 2);
	for (auto const &pr : edgeSet)
	{
		lineIndices.push_back(static_cast<unsigned int>(pr.first));
		lineIndices.push_back(static_cast<unsigned int>(pr.second));
	}

	edgeIndexCount = static_cast<GLsizei>(lineIndices.size());
	if (edgeIBO != 0)
		glDeleteBuffers(1, &edgeIBO);
	glGenBuffers(1, &edgeIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndices.size() * sizeof(unsigned int), lineIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::computeThreeBSP()
{
	threeBSP = make_shared<ThreeBSP>(ThreeBSP(shared_from_this()));
	threeBSPDone = true;
}

shared_ptr<Mesh> Mesh::subtract(shared_ptr<Mesh> const &other)
{
	if (!threeBSPDone)
		computeThreeBSP();
	if (!other->threeBSPDone)
		other->computeThreeBSP();

	auto csg = threeBSP->subtract(other->getThreeBSP());
	std::shared_ptr<CSGMesh> csgObj = csg->toMesh();
	return csgObj;
}

shared_ptr<Mesh> Mesh::add(shared_ptr<Mesh> const &other)
{
	if (!threeBSPDone)
		computeThreeBSP();
	if (!other->threeBSPDone)
		other->computeThreeBSP();

	auto csg = threeBSP->add(other->getThreeBSP());
	std::shared_ptr<CSGMesh> csgObj = csg->toMesh();
	return csgObj;
}

shared_ptr<Mesh> Mesh::intersect(shared_ptr<Mesh> const &other)
{
	if (!threeBSPDone)
		computeThreeBSP();
	if (!other->threeBSPDone)
		other->computeThreeBSP();

	auto csg = threeBSP->intersect(other->getThreeBSP());
	std::shared_ptr<CSGMesh> csgObj = csg->toMesh();
	return csgObj;
}

void Mesh::translate(glm::vec3 const &translation)
{
	threeBSPDone = false;
	*model = glm::translate(*model, translation);
}

void Mesh::rotate(glm::vec3 const &rotation)
{
	threeBSPDone = false;
	*model = glm::rotate(*model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
	*model = glm::rotate(*model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
	*model = glm::rotate(*model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

void Mesh::scale(glm::vec3 const &scale)
{
	threeBSPDone = false;
	*model = glm::scale(*model, scale);
}

glm::f32 *Mesh::getModelPtr()
{
	return glm::value_ptr(*model);
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
	ensureVertexNormals();

	indexCount = static_cast<GLsizei>(indices.size());

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	std::vector<GLfloat> interleaved;
	interleaved.reserve(vertices.size() * 6);
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		interleaved.push_back(vertices[i].x);
		interleaved.push_back(vertices[i].y);
		interleaved.push_back(vertices[i].z);
		glm::vec3 n = i < normals.size() ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
		interleaved.push_back(n.x);
		interleaved.push_back(n.y);
		interleaved.push_back(n.z);
	}

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(GLfloat), interleaved.data(), GL_STATIC_DRAW);

	const GLsizei stride = 6 * static_cast<GLsizei>(sizeof(GLfloat));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Keep IBO bound to this VAO (do not bind 0 here).
	glBindVertexArray(0);

	buildWireframeEdgeBuffer();
}

void Mesh::RenderMesh()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::RenderWireframe()
{
	if (edgeIndexCount <= 0 || edgeIBO == 0)
		return;
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	const GLsizei stride = 6 * static_cast<GLsizei>(sizeof(GLfloat));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void *>(0));
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeIBO);
	glDrawElements(GL_LINES, edgeIndexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::ClearMesh()
{
	if (edgeIBO != 0)
	{
		glDeleteBuffers(1, &edgeIBO);
		edgeIBO = 0;
	}
	edgeIndexCount = 0;

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
