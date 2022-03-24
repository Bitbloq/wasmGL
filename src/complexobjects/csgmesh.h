#ifndef CSGMESH_H
#define CSGMESH_H

#include "../core/mesh.h"

class CSGMesh : public Mesh
{
public:
  CSGMesh(){};
  ~CSGMesh(){};

private:
  void createVertices(){};
};

#endif