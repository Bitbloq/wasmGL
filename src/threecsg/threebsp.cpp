#include "threebsp.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <vector>
#include "glm/glm.hpp"
#include <glm/gtc/matrix_inverse.hpp>
#include "../complexobjects/csgmesh.h"
#include <utility>

using namespace std;

namespace
{

float cross2(glm::vec2 const &a, glm::vec2 const &b, glm::vec2 const &c)
{
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

float signedArea2(vector<glm::vec2> const &p)
{
	float a = 0.f;
	for (size_t i = 0; i < p.size(); ++i)
	{
		size_t const j = (i + 1) % p.size();
		a += p[i].x * p[j].y - p[j].x * p[i].y;
	}
	return a * 0.5f;
}

/** Strict interior (CCW triangle abc); avoids rejecting valid ears when verts lie on edges. */
bool pointInsideTriStrict(glm::vec2 const &p, glm::vec2 const &a, glm::vec2 const &b, glm::vec2 const &c)
{
	float const e = 1e-6f;
	float const s1 = cross2(a, b, p);
	float const s2 = cross2(b, c, p);
	float const s3 = cross2(c, a, p);
	return (s1 > e) && (s2 > e) && (s3 > e);
}

vector<glm::vec3> collapseConsecutive(vector<glm::vec3> const &in, float const eps2)
{
	vector<glm::vec3> out;
	for (auto const &p : in)
	{
		if (out.empty() || glm::dot(p - out.back(), p - out.back()) > eps2)
			out.push_back(p);
	}
	if (out.size() >= 3 && glm::dot(out.front() - out.back(), out.front() - out.back()) <= eps2)
		out.pop_back();
	return out;
}

vector<array<int, 3>> earClip2d(vector<glm::vec2> const &p2)
{
	int const n0 = static_cast<int>(p2.size());
	vector<int> V(n0);
	iota(V.begin(), V.end(), 0);
	vector<array<int, 3>> tris;
	int guard = 0;
	while (static_cast<int>(V.size()) > 3 && guard++ < 1000000)
	{
		bool found = false;
		int const nv = static_cast<int>(V.size());
		for (int i = 0; i < nv; ++i)
		{
			int const i0 = V[(i + nv - 1) % nv];
			int const i1 = V[i];
			int const i2 = V[(i + 1) % nv];
			glm::vec2 const &a = p2[i0];
			glm::vec2 const &b = p2[i1];
			glm::vec2 const &c = p2[i2];
			if (cross2(a, b, c) <= 1e-12f)
				continue;
			bool empty = true;
			for (int k : V)
			{
				if (k == i0 || k == i1 || k == i2)
					continue;
				if (pointInsideTriStrict(p2[k], a, b, c))
				{
					empty = false;
					break;
				}
			}
			if (!empty)
				continue;
			tris.push_back({i0, i1, i2});
			V.erase(V.begin() + i);
			found = true;
			break;
		}
		if (!found)
			break;
	}
	if (V.size() == 3)
		tris.push_back({V[0], V[1], V[2]});
	return tris;
}

/** Simple polygons only (BSP output); handles concave clips — unlike triangle fans. */
vector<array<glm::vec3, 3>> triangulatePlanarNgon(vector<glm::vec3> pts, glm::vec3 const &nMesh)
{
	vector<array<glm::vec3, 3>> out;
	pts = collapseConsecutive(pts, 1e-14f);
	if (pts.size() < 3)
		return out;
	if (pts.size() == 3)
	{
		out.push_back({pts[0], pts[1], pts[2]});
		return out;
	}

	glm::vec3 const n = glm::length(nMesh) > 1e-20f ? glm::normalize(nMesh) : glm::vec3(0.f, 1.f, 0.f);
	glm::vec3 u = glm::cross(n, glm::abs(n.x) < 0.9f ? glm::vec3(1.f, 0.f, 0.f) : glm::vec3(0.f, 1.f, 0.f));
	if (glm::dot(u, u) < 1e-20f)
		u = glm::cross(n, glm::vec3(0.f, 0.f, 1.f));
	u = glm::normalize(u);
	glm::vec3 const v = glm::normalize(glm::cross(u, n));

	vector<glm::vec2> p2(pts.size());
	auto rebuildP2 = [&]()
	{
		glm::vec3 const o = pts[0];
		for (size_t i = 0; i < pts.size(); ++i)
		{
			glm::vec3 const d = pts[i] - o;
			p2[i] = glm::vec2(glm::dot(d, u), glm::dot(d, v));
		}
	};
	rebuildP2();
	if (signedArea2(p2) < 0.f)
	{
		reverse(pts.begin(), pts.end());
		rebuildP2();
	}

	vector<array<int, 3>> tris = earClip2d(p2);
	size_t const expected = pts.size() >= 3 ? pts.size() - 2 : 0;
	if (tris.size() != expected && pts.size() >= 3)
	{
		tris.clear();
		for (size_t j = 2; j < pts.size(); ++j)
			tris.push_back({0, static_cast<int>(j - 1), static_cast<int>(j)});
	}
	for (auto const &t : tris)
		out.push_back({pts[static_cast<size_t>(t[0])], pts[static_cast<size_t>(t[1])], pts[static_cast<size_t>(t[2])]});
	return out;
}

} // namespace

ThreeBSP::ThreeBSP(shared_ptr<Node> const &node)
{
  this->_polygon = make_shared<Polygon>();
  this->_node = make_shared<Node>();
  this->_vertex = make_shared<Vertex>();
  this->tree = node;
  this->matrix = make_shared<glm::mat4>(glm::mat4(1.0f));
}

ThreeBSP::ThreeBSP(shared_ptr<Mesh> const &mesh)
{
  _polygon = make_shared<Polygon>();
  _node = make_shared<Node>();
  _vertex = make_shared<Vertex>();

  /** Own a copy so BSP geometry and `toMesh()` inverse stay aligned with the transform used here,
   *  even if `mesh->model` is mutated before the next `computeThreeBSP()`. */
  matrix = make_shared<glm::mat4>(*mesh->getModelMatrix());
  auto geometry = mesh;
  vector<shared_ptr<Polygon>> polygons;
  polygons.reserve(geometry->faces.size());

  for (size_t i{0}; i < geometry->faces.size(); i++)
  {
    auto face = geometry->faces.at(i);
    // auto faceVertexUvs = geometry->faceVertexUvs.at(0).at(i);
    auto polygon = make_shared<Polygon>();

    auto geomvertex = geometry->vertices.at(face->a);
    // auto uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(0)->x, faceVertexUvs.at(0)->y));
    // auto uvs = nullptr;
    auto vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(0) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    geomvertex = geometry->vertices.at(face->b);
    // uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(1)->x, faceVertexUvs.at(1)->y));
    // uvs = nullptr;
    vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(1) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    geomvertex = geometry->vertices.at(face->c);
    // uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(2)->x, faceVertexUvs.at(2)->y));
    // uvs = nullptr;
    vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(2) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    polygon->calculateProperties();
    polygons.push_back(polygon);
  }
  this->tree = make_shared<Node>(Node(polygons));
}

shared_ptr<ThreeBSP> ThreeBSP::subtract(shared_ptr<ThreeBSP> const &other_tree)
{
  auto a = this->tree->clone();
  auto b = other_tree->tree->clone();

  a->invert();
  a->clipTo(b);
  b->clipTo(a);
  b->invert();
  b->clipTo(a);
  b->invert();
  a->build(b->allPolygons());
  a->invert();
  auto absp = make_shared<ThreeBSP>(ThreeBSP(a));
  absp->matrix = this->matrix;
  return absp;
}

shared_ptr<ThreeBSP> ThreeBSP::add(shared_ptr<ThreeBSP> const &other_tree)
{
  auto a = this->tree->clone();
  auto b = other_tree->tree->clone();
  a->clipTo(b);
  b->clipTo(a);
  b->invert();
  b->clipTo(a);
  b->invert();
  a->build(b->allPolygons());
  auto absp = make_shared<ThreeBSP>(ThreeBSP(a));
  absp->matrix = this->matrix;
  return absp;
}

shared_ptr<ThreeBSP> ThreeBSP::intersect(shared_ptr<ThreeBSP> const &other_tree)
{
  auto a = this->tree->clone();
  auto b = other_tree->tree->clone();
  a->invert();
  b->clipTo(a);
  b->invert();
  a->clipTo(b);
  b->clipTo(a);
  a->build(b->allPolygons());
  a->invert();
  auto absp = make_shared<ThreeBSP>(ThreeBSP(a));
  absp->matrix = this->matrix;

  return absp;
}

shared_ptr<CSGMesh> ThreeBSP::toMesh()
{
  /** BSP polygons live in world space (built from meshes with model applied). Bake into
   *  the first operand's model space using inverse(model), but the rendered mesh must use
   *  the original model matrix — setting model to inverse(model) was double-applying the
   *  inverse and broke unions whenever operands were moved or rotated. */
  auto const invModelPtr = make_shared<glm::mat4x4>(glm::mat4x4(glm::inverse(*this->matrix)));
  glm::mat3 const normalToMesh = glm::mat3(glm::inverse(*this->matrix));
  auto mesh = make_shared<CSGMesh>();
  auto polygons = this->tree->allPolygons();
  size_t faceTotal = 0;
  for (auto const &poly : polygons)
  {
    if (poly->vertices.size() > 2)
      faceTotal += poly->vertices.size() - 2;
  }
  mesh->faces.reserve(faceTotal);
  mesh->indices.reserve(faceTotal * 3);

  for (size_t i{0}; i < polygons.size(); i++)
  {
    auto polygon = polygons.at(i);
    glm::vec3 const nWorld(polygon->normal->position.x, polygon->normal->position.y, polygon->normal->position.z);
    glm::vec3 const nMesh = glm::normalize(normalToMesh * nWorld);

    vector<glm::vec3> meshRing;
    meshRing.reserve(polygon->vertices.size());
    for (auto const &vv : polygon->vertices)
    {
      auto const w = make_shared<glm::vec3>(vv->position);
      meshRing.push_back(*applyMatrix4(w, invModelPtr));
    }

    vector<array<glm::vec3, 3>> const tris = triangulatePlanarNgon(meshRing, nMesh);
    for (auto const &tri : tris)
    {
      glm::vec3 pa = tri[0];
      glm::vec3 pb = tri[1];
      glm::vec3 pc = tri[2];

      glm::vec3 const geomN = glm::cross(pb - pa, pc - pa);
      if (glm::dot(geomN, geomN) > 1e-24f && glm::dot(geomN, nMesh) < 0.0f)
        std::swap(pb, pc);

      unsigned int const vertex_idx_a = static_cast<unsigned int>(mesh->vertices.size());
      mesh->vertices.push_back(pa);
      unsigned int const vertex_idx_b = static_cast<unsigned int>(mesh->vertices.size());
      mesh->vertices.push_back(pb);
      unsigned int const vertex_idx_c = static_cast<unsigned int>(mesh->vertices.size());
      mesh->vertices.push_back(pc);

      auto face = make_shared<Face3>(Face3(static_cast<int>(vertex_idx_a),
                                           static_cast<int>(vertex_idx_b),
                                           static_cast<int>(vertex_idx_c),
                                           nMesh));
      mesh->faces.push_back(face);
    }
  }
  // faces to indices
  for (auto face : mesh->faces)
  {
    mesh->indices.push_back(face->a);
    mesh->indices.push_back(face->b);
    mesh->indices.push_back(face->c);
  }
  mesh->setModel(make_shared<glm::mat4>(*this->matrix));
  mesh->createMesh();
  mesh->computeThreeBSP();

  return mesh;
}

shared_ptr<glm::vec3> applyMatrix4(shared_ptr<glm::vec3> const &v, shared_ptr<glm::mat4x4> const &m)
{
  auto elements = glm::value_ptr(*m);
  auto w = 1 / (elements[3] * v->x + elements[7] * v->y + elements[11] * v->z + elements[15]);
  auto x = (elements[0] * v->x + elements[4] * v->y + elements[8] * v->z + elements[12]) * w;
  auto y = (elements[1] * v->x + elements[5] * v->y + elements[9] * v->z + elements[13]) * w;
  auto z = (elements[2] * v->x + elements[6] * v->y + elements[10] * v->z + elements[14]) * w;
  return make_shared<glm::vec3>(glm::vec3(x, y, z));
}
