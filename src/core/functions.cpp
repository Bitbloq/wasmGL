#include <memory>
#include "functions.h"

std::shared_ptr<Box> createBox(BoxDimensions const &dimensions)
{
  auto mesh = std::make_shared<Box>(dimensions);
  return mesh;
}

std::shared_ptr<Sphere> createSphere(SphereDimensions const &dimensions, SphereParameters const &parameters)
{
  auto mesh = std::make_shared<Sphere>(dimensions, parameters);
  return mesh;
}

std::shared_ptr<Pyramid> createPyramid(PyramidDimensions const &dimensions)
{
  auto mesh = std::make_shared<Pyramid>(dimensions);
  return mesh;
}

std::shared_ptr<Cylinder> createCylinder(CylinderDimensions const &dimensions, CylinderParameters const &parameters)
{
  auto mesh = std::make_shared<Cylinder>(dimensions, parameters);
  return mesh;
}

std::shared_ptr<Torus> createTorus(TorusDimensions const &dimensions, TorusParameters const &parameters)
{
  auto mesh = std::make_shared<Torus>(dimensions, parameters);
  return mesh;
}

std::shared_ptr<Mesh> rotate(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &rotation)
{
  mesh->rotate(rotation);
  mesh->threeBSPDone = false;
  return mesh;
}

std::shared_ptr<Mesh> translate(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &translation)
{
  mesh->translate(translation);
  mesh->threeBSPDone = false;
  return mesh;
}

std::shared_ptr<Mesh> scale(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &scaleVec)
{
  mesh->scale(scaleVec);
  mesh->threeBSPDone = false;
  return mesh;
}
