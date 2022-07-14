#ifndef CREATE_FUNTIONS_H
#define CREATE_FUNTIONS_H

#include <memory>
#include "../core/mesh.h"
#include "./box.h"
#include "./sphere.h"

using namespace std;

std::shared_ptr<Box> createBox(BoxDimensions const &dimensions);
std::shared_ptr<Sphere> createSphere(SphereDimensions const &dimensions, SphereParameters const &parameters);

#endif // CREATE_FUNTIONS_H