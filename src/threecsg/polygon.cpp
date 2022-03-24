#include "polygon.h"
#include "types.h"

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
  auto a = vertices.at(0);
  auto b = vertices.at(1);
  auto c = vertices.at(2);

  this->normal = b->clone()->subtract(a)->cross(c->clone()->subtract(a))->normalize();
  this->w = this->normal->clone()->dot(a);
}

shared_ptr<Polygon> Polygon::clone() const
{
  auto polygon = make_shared<Polygon>(Polygon());
  for (auto vertex : vertices)
  {
    polygon->vertices.push_back(vertex->clone());
  }
  polygon->calculateProperties();
  return polygon;
}

shared_ptr<Polygon> Polygon::flip()
{
  vector<shared_ptr<Vertex>> vertices_clone;
  this->normal = this->normal->multiplyScalar(-1);
  this->w *= -1;
  for (int i = this->vertices.size() - 1; i >= 0; i--)
  {
    vertices_clone.push_back(this->vertices.at(i));
  }
  this->vertices = vertices_clone;
  return shared_from_this();
}

CLASSIFICATION Polygon::classifyVertex(shared_ptr<Vertex> const &vertex)
{
  auto side_value = this->normal->dot(vertex) - this->w;
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
  auto vertices_count = polygon->vertices.size();
  for (size_t i{0}; i < vertices_count; i++)
  {
    auto vertex = polygon->vertices.at(i);
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
  else
  {
    return SPANNING;
  }
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
    for (size_t i{0}; i < polygon->vertices.size(); i++)
    {
      auto j = (i + 1) % polygon->vertices.size();
      auto vi = polygon->vertices.at(i);
      auto vj = polygon->vertices.at(j);
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
        auto t = (this->w - this->normal->dot(vi)) / (this->normal->dot(vj->clone()->subtract(vi)));
        auto v = vi->interpolate(vj, t);
        front_vertices.push_back(v);
        back_vertices.push_back(v);
      }
    }
    if (front_vertices.size() >= 3)
    {
      auto p = make_shared<Polygon>(front_vertices);
      p->calculateProperties();
      front.push_back(p);
    }
    if (back_vertices.size() >= 3)
    {
      auto p = make_shared<Polygon>(back_vertices);
      p->calculateProperties();
      back.push_back(p);
    }
  }
}
