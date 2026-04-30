#include "polygon.h"
#include "types.h"
#include <algorithm>
#include <glm/glm.hpp>

Polygon::Polygon(vector<shared_ptr<Vertex>> const &vertices)
    : vertices(vertices)
{
  if (vertices.size() >= 3)
  {
    calculateProperties();
  }
  else
  {
    normal = nullptr;
    w = 0;
  }
}

void Polygon::calculateProperties()
{
  auto const &a = vertices.at(0);
  auto const &b = vertices.at(1);
  auto const &c = vertices.at(2);

  glm::vec3 const e1 = b->position - a->position;
  glm::vec3 const e2 = c->position - a->position;
  glm::vec3 const n = glm::normalize(glm::cross(e1, e2));
  this->normal = std::make_shared<Vertex>(Vertex(n, glm::vec3(), glm::vec2()));
  this->w = glm::dot(n, a->position);
}

shared_ptr<Polygon> Polygon::clone() const
{
  auto polygon = make_shared<Polygon>(Polygon());
  polygon->vertices.reserve(vertices.size());
  for (auto const &vertex : vertices)
    polygon->vertices.push_back(vertex->clone());
  polygon->calculateProperties();
  return polygon;
}

shared_ptr<Polygon> Polygon::flip()
{
  this->normal = this->normal->multiplyScalar(-1);
  this->w *= -1;
  std::reverse(this->vertices.begin(), this->vertices.end());
  return shared_from_this();
}

CLASSIFICATION Polygon::classifyVertex(shared_ptr<Vertex> const &vertex)
{
  float const side_value = glm::dot(this->normal->position, vertex->position) - this->w;
  if (side_value < -EPSILON)
  {
    return BACK;
  }

  if (side_value > EPSILON)
  {
    return FRONT;
  }

  return COPLANAR;
}

CLASSIFICATION Polygon::classifySide(shared_ptr<Polygon> const &polygon)
{
  int num_positive{0};
  int num_negative{0};
  for (auto const &vertex : polygon->vertices)
  {
    auto classification = classifyVertex(vertex);
    if (classification == FRONT)
    {
      num_positive++;
    }
    else if (classification == BACK)
    {
      num_negative++;
    }
  }
  if (num_positive > 0 && num_negative == 0)
  {
    return FRONT;
  }
  if (num_positive == 0 && num_negative > 0)
  {
    return BACK;
  }
  if (num_positive == 0 && num_negative == 0)
  {
    return COPLANAR;
  }
  return SPANNING;
}

void Polygon::splitPolygon(shared_ptr<Polygon> const &polygon, vector<shared_ptr<Polygon>> &coplanar_front, vector<shared_ptr<Polygon>> &coplanar_back, vector<shared_ptr<Polygon>> &front, vector<shared_ptr<Polygon>> &back)
{

  auto classification = classifySide(polygon);
  if (classification == COPLANAR)
  {
    if (this->normal->dot(polygon->normal) > 0)
    {
      coplanar_front.push_back(polygon);
    }
    else
    {
      coplanar_back.push_back(polygon);
    }
  }
  else if (classification == FRONT)
  {
    front.push_back(polygon);
  }
  else if (classification == BACK)
  {
    back.push_back(polygon);
  }
  else
  {
    vector<shared_ptr<Vertex>> front_vertices;
    vector<shared_ptr<Vertex>> back_vertices;
    size_t const n = polygon->vertices.size();
    front_vertices.reserve(n + 2);
    back_vertices.reserve(n + 2);
    glm::vec3 const planeN = this->normal->position;
    for (size_t i{0}; i < n; i++)
    {
      size_t const j = (i + 1) % n;
      auto const &vi = polygon->vertices.at(i);
      auto const &vj = polygon->vertices.at(j);
      auto ti = this->classifyVertex(vi);
      auto tj = this->classifyVertex(vj);
      if (ti != BACK)
      {
        front_vertices.push_back(vi);
      }
      if (ti != FRONT)
      {
        back_vertices.push_back(vi);
      }
      if ((ti | tj) == CLASSIFICATION::SPANNING)
      {
        glm::vec3 const edge = vj->position - vi->position;
        float const denom = glm::dot(planeN, edge);
        float const t = (this->w - glm::dot(planeN, vi->position)) / denom;
        auto v = vi->interpolate(vj, t);
        front_vertices.push_back(v);
        back_vertices.push_back(v);
      }
    }
    if (front_vertices.size() >= 3)
    {
      auto p = make_shared<Polygon>(front_vertices);
      // p->calculateProperties();
      front.push_back(p);
    }
    if (back_vertices.size() >= 3)
    {
      auto p = make_shared<Polygon>(back_vertices);
      // p->calculateProperties();
      back.push_back(p);
    }
  }
}
