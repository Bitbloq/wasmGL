#include "vertex.h"

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
shared_ptr<Vertex> Vertex::add(shared_ptr<Vertex> p)
{
  position += p->position;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::subtract(shared_ptr<Vertex> p)
{
  position -= p->position;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::multiplyScalar(float s)
{
  position *= s;
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::cross(shared_ptr<Vertex> p)
{
  position = glm::cross(position, p->position);
  return shared_from_this();
}
shared_ptr<Vertex> Vertex::normalize()
{
  position = glm::normalize(position);
  return shared_from_this();
}
float Vertex::dot(shared_ptr<Vertex> p) const
{
  return glm::dot(position, p->position);
}

shared_ptr<Vertex> Vertex::lerp(shared_ptr<Vertex> a, float t)
{
  this->add(a->clone()->subtract(shared_from_this())->multiplyScalar(t));
  glm::vec3 aux3 = t * (a->normal - this->normal);
  this->normal += aux3;

  glm::vec2 aux2 = t * (a->uv - this->uv);
  this->uv += aux2;

  return shared_from_this();
}

shared_ptr<Vertex> Vertex::interpolate(shared_ptr<Vertex> other, float t)
{
  return this->clone()->lerp(other, t);
}

shared_ptr<Vertex> Vertex::applyMatrix(glm::mat4 const &matrix)
{
  float x = position.x;
  float y = position.y;
  float z = position.z;

  position.x = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z + matrix[0][3];
  position.y = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z + matrix[1][3];
  position.z = matrix[2][0] * x + matrix[2][1] * y + matrix[2][2] * z + matrix[2][3];

  return shared_from_this();
}