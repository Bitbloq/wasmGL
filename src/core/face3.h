#ifndef FACE3_H
#define FACE3_H

#include <memory>
#include <glm/glm.hpp>
#include <array>

using namespace std;

class Face3 : public std::enable_shared_from_this<Face3>
{
public:
  Face3(int a, int b, int c, glm::vec3 const &normal = glm::vec3(), std::array<glm::vec3, 3> const &vertexNormals = {});

  shared_ptr<Face3> clone() const;
  shared_ptr<Face3> copy(shared_ptr<Face3> const &face);

  int a, b, c;                            // vertex a,b,c indices
  glm::vec3 normal;                       // face normal
  std::array<glm::vec3, 3> vertexNormals; // vertex normals
};

#endif // FACE3_H