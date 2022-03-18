#ifndef NODE_H
#define NODE_H

#include <memory>
#include <vector>

#include "polygon.h"

using namespace std;

class Node : public std::enable_shared_from_this<Node>
{
public:
  Node(vector<shared_ptr<Polygon>> const &polygons = {});
  bool isConvex(vector<shared_ptr<Polygon>> const &polygons);
  void build(vector<shared_ptr<Polygon>> const &polygons);
  vector<shared_ptr<Polygon>> allPolygons() const;
  shared_ptr<Node> clone() const;
  shared_ptr<Node> invert();
  vector<shared_ptr<Polygon>> clipPolygons(vector<shared_ptr<Polygon>> const &polygons) const;
  void clipTo(shared_ptr<Node> const &node);

private:
  vector<shared_ptr<Polygon>> polygons;
  shared_ptr<Node> front;
  shared_ptr<Node> back;
  shared_ptr<Polygon> divider;
};

#endif