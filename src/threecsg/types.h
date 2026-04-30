#ifndef TYPES_H
#define TYPES_H

/** Plane tests / clipping; slightly relaxed vs 1e-5 for float CSG after transforms. */
#define EPSILON 2e-4f

enum CLASSIFICATION
{
  COPLANAR = 0,
  FRONT = 1,
  BACK = 2,
  SPANNING = 3
};

#endif