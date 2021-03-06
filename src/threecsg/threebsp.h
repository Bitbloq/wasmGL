#ifndef THREE_BSP_H
#define THREE_BSP_H

#include <memory>
#include <set>
#include "polygon.h"
#include "vertex.h"
#include "node.h"
#include "../core/mesh.h"
#include "../complexobjects/csgmesh.h"

#include <glm/mat4x4.hpp>

using namespace std;

class ThreeBSP : public std::enable_shared_from_this<ThreeBSP>
{
public:
  ThreeBSP(shared_ptr<Mesh> const &mesh);
  ThreeBSP(shared_ptr<Node> const &node);
  shared_ptr<ThreeBSP> subtract(shared_ptr<ThreeBSP> const &other_tree);
  shared_ptr<ThreeBSP> add(shared_ptr<ThreeBSP> const &other_tree);
  shared_ptr<ThreeBSP> intersect(shared_ptr<ThreeBSP> const &other_tree);

  shared_ptr<CSGMesh> toMesh();

private:
  shared_ptr<Polygon> _polygon;
  shared_ptr<Node> _node;
  shared_ptr<Vertex> _vertex;
  shared_ptr<glm::mat4> matrix;
  shared_ptr<Node> tree;
};

shared_ptr<glm::vec3> applyMatrix4(shared_ptr<glm::vec3> const &v, shared_ptr<glm::mat4x4> const &m);
bool exists(set<pair<string, int>> const &data, string key);
int getValue(set<pair<string, int>> const &data, string key);
string make_key(shared_ptr<glm::vec3> const &v);
set<pair<string, int>> &addPair(set<pair<string, int>> &data, string key, int value);

bool operator==(pair<string, int> const &a, pair<string, int> const &b);
bool operator<(pair<string, int> const &a, pair<string, int> const &b);
bool operator>(pair<string, int> const &a, pair<string, int> const &b);
bool operator!=(pair<string, int> const &a, pair<string, int> const &b);
bool operator<=(pair<string, int> const &a, pair<string, int> const &b);
bool operator>=(pair<string, int> const &a, pair<string, int> const &b);

#endif
