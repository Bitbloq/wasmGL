#include "../threecsg/threebsp.h"
#include <memory>
#include <thread>
#include <functional>
#include "functions.h"

std::shared_ptr<Box> createBox(BoxDimensions const &dimensions)
{
  auto computeThreeBSP = [](shared_ptr<Mesh> const &mesh)
  {
    auto threeBSP = make_shared<ThreeBSP>(ThreeBSP(mesh));
    mesh->setThreeBSP(threeBSP);
    mesh->threeBSPDone = true;
  };

  auto mesh = std::make_shared<Box>(dimensions);
  mesh->threeBSPDone = false;
  std::thread thread_object(computeThreeBSP, mesh);
  thread_object.detach();
  return mesh;
}

std::shared_ptr<Sphere> createSphere(SphereDimensions const &dimensions, SphereParameters const &parameters)
{
  auto computeThreeBSP = [](shared_ptr<Mesh> const &mesh)
  {
    auto threeBSP = make_shared<ThreeBSP>(ThreeBSP(mesh));
    mesh->setThreeBSP(threeBSP);
    mesh->threeBSPDone = true;
  };

  auto mesh = std::make_shared<Sphere>(dimensions, parameters);
  mesh->threeBSPDone = false;
  std::thread thread_object(computeThreeBSP, mesh);
  thread_object.detach();
  return mesh;
}

std::shared_ptr<Mesh> rotate(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &rotation)
{
  auto computeThreeBSP = [](shared_ptr<Mesh> const &mesh)
  {
    auto threeBSP = make_shared<ThreeBSP>(ThreeBSP(mesh));
    mesh->setThreeBSP(threeBSP);
    mesh->threeBSPDone = true;
  };

  mesh->rotate(rotation);
  mesh->threeBSPDone = false;
  std::thread thread_object(computeThreeBSP, mesh);
  thread_object.detach();
  return mesh;
}

std::shared_ptr<Mesh> translate(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &translation)
{
  auto computeThreeBSP = [](shared_ptr<Mesh> const &mesh)
  {
    auto threeBSP = make_shared<ThreeBSP>(ThreeBSP(mesh));
    mesh->setThreeBSP(threeBSP);
    mesh->threeBSPDone = true;
  };

  mesh->translate(translation);
  mesh->threeBSPDone = false;
  std::thread thread_object(computeThreeBSP, mesh);
  thread_object.detach();
  return mesh;
}
