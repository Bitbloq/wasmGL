#ifndef POLYGON_H
#define POLYGON_H

#include <memory>
#include <vector>
#include "vertex.h"
#include "types.h"

using namespace std;

class Polygon : public std::enable_shared_from_this<Polygon>
{
public:
  Polygon(vector<shared_ptr<Vertex>> const &vertices = {});
  shared_ptr<Polygon> clone() const;
  shared_ptr<Polygon> calculateProperties();
  shared_ptr<Polygon> flip();
  CLASSIFICATION classifyVertex(shared_ptr<Vertex> const &vertex);
  CLASSIFICATION classifySide(shared_ptr<Polygon> const &polygon);
  void splitPolygon(shared_ptr<Polygon> const &polygon, vector<shared_ptr<Polygon>> &coplanar_front, vector<shared_ptr<Polygon>> &coplanar_back, vector<shared_ptr<Polygon>> &front, vector<shared_ptr<Polygon>> &back);

  vector<shared_ptr<Vertex>> vertices;
  shared_ptr<Vertex> normal;
  float w;
};

#endif