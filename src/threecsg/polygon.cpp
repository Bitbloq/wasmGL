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
    normal = {};
    w = 0;
  }
}

shared_ptr<Polygon> Polygon::calculateProperties()
{
  auto a = vertices.at(0);
  auto b = vertices.at(1);
  auto c = vertices.at(2);

  normal = b->clone()->subtract(a)->cross(c->clone()->subtract(a))->normalize();
  w = normal->clone()->dot(a);

  return shared_from_this();
}

shared_ptr<Polygon> Polygon::clone() const
{
  vector<shared_ptr<Vertex>> vertices_clone;
  for (auto vertex : vertices)
  {
    vertices_clone.push_back(vertex->clone());
  }
  return make_shared<Polygon>(Polygon(vertices_clone));
}

shared_ptr<Polygon> Polygon::flip()
{
  vector<shared_ptr<Vertex>> vertices_clone;
  normal = normal->multiplyScalar(-1);
  w = w * -1;
  for (int i = vertices.size() - 1; i >= 0; i--)
  {
    vertices_clone.push_back(vertices.at(i));
  }
  vertices = vertices_clone;
  return shared_from_this();
}

CLASSIFICATION Polygon::classifyVertex(shared_ptr<Vertex> const &vertex)
{
  auto side_value = normal->dot(vertex) - w;
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
  int num_positive = 0;
  int num_negative = 0;
  for (int i{0}; i < vertices.size(); i++)
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
  else if (num_positive == 0 && num_negative > 0)
  {
    return BACK;
  }
  else if (num_positive == 0 && num_negative == 0)
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
      if (tj != FRONT)
      {
        back_vertices.push_back(vj);
      }
      if (ti | tj == CLASSIFICATION::SPANNING)
      {
        auto t = (this->w - this->normal->dot(vi)) / (this->normal->dot(vj->clone()->subtract(vi)));
        auto v = vi->interpolate(vj, t);
        front_vertices.push_back(v);
        back_vertices.push_back(v);
      }
    }
    if (front_vertices.size() >= 3)
    {
      front.push_back(make_shared<Polygon>(Polygon(front_vertices)));
    }
    if (back_vertices.size() >= 3)
    {
      back.push_back(make_shared<Polygon>(Polygon(back_vertices)));
    }
  }
}