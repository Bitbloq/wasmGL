#include "vertex.h"
#include <glm/glm.hpp>
#include "glm/gtc/type_ptr.hpp"

Vertex::Vertex(glm::vec3 const &position, glm::vec3 const &normal, glm::vec2 const &uv)
    : position(position),
      normal(normal),
      uv(uv)
{
}

shared_ptr<Vertex> Vertex::clone() const
{
  return make_shared<Vertex>(Vertex(position, normal, uv));
}
shared_ptr<Vertex> Vertex::add(shared_ptr<Vertex> const &p)
{
  position += p->position;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::subtract(shared_ptr<Vertex> const &p)
{
  position -= p->position;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::multiplyScalar(float s)
{
  position *= s;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::cross(shared_ptr<Vertex> const &p)
{
  position = glm::cross(position, p->position);
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::normalize()
{
  position = glm::normalize(position);
  return shared_from_this();
}
float Vertex::dot(shared_ptr<Vertex> const &p) const
{
  return glm::dot(position, p->position);
}

shared_ptr<Vertex> Vertex::lerp(shared_ptr<Vertex> const &a, float t)
{
  this->add(a->clone()->subtract(shared_from_this())->multiplyScalar(t));
  glm::vec3 aux3 = t * (a->normal - this->normal);
  this->normal += aux3;

  glm::vec2 aux2 = t * (a->uv - this->uv);
  this->uv += aux2;

  return shared_from_this();
}

shared_ptr<Vertex> Vertex::interpolate(shared_ptr<Vertex> const &other, float t)
{
  return this->clone()->lerp(other, t);
}

shared_ptr<Vertex> Vertex::applyMatrix(glm::mat4 const &matrix)
{
  float x = position.x;
  float y = position.y;
  float z = position.z;

  auto e = glm::value_ptr(matrix);

  position.x = e[0] * x + e[4] * y + e[8] * z + e[12];
  position.y = e[1] * x + e[5] * y + e[9] * z + e[13];
  position.z = e[2] * x + e[6] * y + e[10] * z + e[14];

  return shared_from_this();
}