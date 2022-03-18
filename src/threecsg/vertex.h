#ifndef VERTEX_H
#define VERTEX_H
#include <memory>
#include "glm/glm.hpp"

using namespace std;

class Vertex : public std::enable_shared_from_this<Vertex>
{
public:
  Vertex(glm::vec3 const &position = glm::vec3(), glm::vec3 const &normal = glm::vec3(), glm::vec2 const &uv = glm::vec2());
  shared_ptr<Vertex> clone() const;
  shared_ptr<Vertex> add(shared_ptr<Vertex> p);
  shared_ptr<Vertex> subtract(shared_ptr<Vertex> p);
  shared_ptr<Vertex> multiplyScalar(float s);
  shared_ptr<Vertex> cross(shared_ptr<Vertex> p);
  shared_ptr<Vertex> normalize();
  float dot(shared_ptr<Vertex> p) const;
  shared_ptr<Vertex> lerp(shared_ptr<Vertex> a, float t);
  shared_ptr<Vertex> interpolate(shared_ptr<Vertex> other, float t);
  shared_ptr<Vertex> applyMatrix(glm::mat4 const &matrix);

  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
};

#endif