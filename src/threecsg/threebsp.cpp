#include "threebsp.h"
#include <vector>
#include "glm/glm.hpp"
#include <map>
#include "../complexobjects/csgmesh.h"
#include <iostream>

using namespace std;

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

  matrix = make_shared<glm::mat4x4>(glm::mat4x4(mesh->getModelMatrix()));
  auto geometry = mesh;
  vector<shared_ptr<Polygon>> polygons;

  for (size_t i{0}; i < geometry->faces.size(); i++)
  {
    auto face = geometry->faces.at(i);
    // auto faceVertexUvs = geometry->faceVertexUvs.at(0).at(i);
    auto polygon = make_shared<Polygon>();

    auto geomvertex = geometry->vertices.at(face->a);
    // auto uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(0)->x, faceVertexUvs.at(0)->y));
    auto uvs = nullptr;
    auto vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(0) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    geomvertex = geometry->vertices.at(face->b);
    // uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(1)->x, faceVertexUvs.at(1)->y));
    uvs = nullptr;
    vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(1) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    geomvertex = geometry->vertices.at(face->c);
    // uvs = make_shared<glm::vec2>(glm::vec2(faceVertexUvs.at(2)->x, faceVertexUvs.at(2)->y));
    uvs = nullptr;
    vertex = make_shared<Vertex>(Vertex(glm::vec3(geomvertex.x, geomvertex.y, geomvertex.z), face->vertexNormals.at(2) /*, *uvs*/));
    vertex->applyMatrix(*matrix);
    polygon->vertices.push_back(vertex);

    polygon->calculateProperties();
    polygons.push_back(polygon);
  }
  cout << polygons.size() << std::endl;
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
  vector<pair<string, int>> vertice_dict;
  auto matrix = make_shared<glm::mat4x4>(glm::mat4x4(*(this->matrix)));
  *matrix = glm::inverse(*matrix);
  auto mesh = make_shared<CSGMesh>();
  auto polygons = this->tree->allPolygons();

  for (size_t i{0}; i < polygons.size(); i++)
  {
    auto polygon = polygons.at(i);
    for (size_t j{2}; j < polygon->vertices.size(); j++)
    {
      std::vector<shared_ptr<glm::vec2>> verticeUvs;
      auto vertex = polygon->vertices.at(0);
      verticeUvs.push_back(make_shared<glm::vec2>(glm::vec2(vertex->uv.x, vertex->uv.y)));
      auto vertex3 = make_shared<glm::vec3>(glm::vec3(vertex->position.x, vertex->position.y, vertex->position.z));
      vertex3 = applyMatrix4(vertex3, matrix);
      int vertex_idx_a;
      if (exists(vertice_dict, make_key(vertex3)))
      {
        vertex_idx_a = getValue(vertice_dict, make_key(vertex3));
      }
      else
      {
        mesh->vertices.push_back(*vertex3);
        addPair(vertice_dict, make_key(vertex3), mesh->vertices.size() - 1);
        vertex_idx_a = mesh->vertices.size() - 1;
      }

      vertex = polygon->vertices.at(j - 1);
      verticeUvs.push_back(make_shared<glm::vec2>(glm::vec2(vertex->uv.x, vertex->uv.y)));
      vertex3 = make_shared<glm::vec3>(glm::vec3(vertex->position.x, vertex->position.y, vertex->position.z));
      vertex3 = applyMatrix4(vertex3, matrix);
      int vertex_idx_b;
      if (exists(vertice_dict, make_key(vertex3)))
      {
        vertex_idx_b = getValue(vertice_dict, make_key(vertex3));
      }
      else
      {
        mesh->vertices.push_back(*vertex3);
        addPair(vertice_dict, make_key(vertex3), mesh->vertices.size() - 1);
        vertex_idx_b = mesh->vertices.size() - 1;
      }

      vertex = polygon->vertices.at(j);
      verticeUvs.push_back(make_shared<glm::vec2>(glm::vec2(vertex->uv.x, vertex->uv.y)));
      vertex3 = make_shared<glm::vec3>(glm::vec3(vertex->position.x, vertex->position.y, vertex->position.z));
      vertex3 = applyMatrix4(vertex3, matrix);
      int vertex_idx_c;
      if (exists(vertice_dict, make_key(vertex3)))
      {
        vertex_idx_c = getValue(vertice_dict, make_key(vertex3));
      }
      else
      {
        mesh->vertices.push_back(*vertex3);
        addPair(vertice_dict, make_key(vertex3), mesh->vertices.size() - 1);
        vertex_idx_c = mesh->vertices.size() - 1;
      }

      auto face = make_shared<Face3>(Face3(vertex_idx_a,
                                           vertex_idx_b,
                                           vertex_idx_c,
                                           glm::vec3(
                                               polygon->normal->position.x,
                                               polygon->normal->position.y,
                                               polygon->normal->position.z)));
      mesh->faces.push_back(face);
      // mesh->faceVertexUvs.at(0).push_back(verticeUvs);
    }
  }
  // faces to indices
  for (auto face : mesh->faces)
  {
    mesh->indices.push_back(face->a);
    mesh->indices.push_back(face->b);
    mesh->indices.push_back(face->c);
  }
  mesh->CreateMesh();
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
bool exists(vector<pair<string, int>> data, string key)
{
  for (auto p : data)
  {
    if (p.first == key)
    {
      return true;
    }
  }
  return false;
}

int getValue(vector<pair<string, int>> data, string key)
{
  for (auto p : data)
  {
    if (p.first == key)
    {
      return p.second;
    }
  }
  return -1;
}
string make_key(shared_ptr<glm::vec3> const &v)
{
  return to_string(v->x) + "," + to_string(v->y) + "," + to_string(v->z);
}

vector<pair<string, int>> &addPair(vector<pair<string, int>> &data, string key, int value)
{
  if (exists(data, key))
  {
    for (auto p : data)
    {
      if (p.first == key)
      {
        p.second = value;
      }
    }
  }
  else
  {
    data.push_back(make_pair(key, value));
  }
  return data;
}
