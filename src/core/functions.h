#ifndef CREATE_FUNTIONS_H
#define CREATE_FUNTIONS_H

#include <memory>
#include "mesh.h"
#include "../primitives/box.h"
#include "../primitives/sphere.h"

using namespace std;

std::shared_ptr<Box> createBox(BoxDimensions const &dimensions);
std::shared_ptr<Sphere> createSphere(SphereDimensions const &dimensions, SphereParameters const &parameters);

std::shared_ptr<Mesh> rotate(std::shared_ptr<Mesh> const &mesh, glm::vec3 const &rotation);

#endif // CREATE_FUNTIONS_H