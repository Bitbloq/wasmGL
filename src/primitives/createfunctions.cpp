#include "box.h"
#include "pyramid.h"
#include "sphere.h"
#include "../threecsg/threebsp.h"
#include <memory>
#include <thread>
#include <functional>
#include "./createfunctions.h"

std::shared_ptr<Box> createBox(BoxDimensions const &dimensions)
{
  auto computeThreeBSP = [](shared_ptr<Mesh> const &mesh)
  {
    auto threeBSP = make_shared<ThreeBSP>(ThreeBSP(mesh));
    mesh->setThreeBSP(threeBSP);
  };

  auto mesh = std::make_shared<Box>(dimensions);
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
  };

  auto mesh = std::make_shared<Sphere>(dimensions, parameters);
  std::thread thread_object(computeThreeBSP, mesh);
  thread_object.detach();
  return mesh;
}