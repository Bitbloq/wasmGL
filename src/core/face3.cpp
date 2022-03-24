#include "face3.h"
#include <memory>

using namespace std;

Face3::Face3(int a, int b, int c, glm::vec3 const &normal, std::array<glm::vec3, 3> const &vertexNormals)
{
  this->a = a;
  this->b = b;
  this->c = c;
  this->normal = normal;
  this->vertexNormals = vertexNormals;
}

shared_ptr<Face3> Face3::clone() const
{
  return make_shared<Face3>(Face3(a, b, c, normal, vertexNormals));
}
shared_ptr<Face3> Face3::copy(shared_ptr<Face3> const &face)
{
  a = face->a;
  b = face->b;
  c = face->c;
  normal = face->normal;
  vertexNormals = face->vertexNormals;
  return shared_from_this();
}