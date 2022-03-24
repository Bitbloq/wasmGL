#include "node.h"
#include <algorithm>

Node::Node(vector<shared_ptr<Polygon>> const &polygons)
{
  this->front = nullptr;
  this->back = nullptr;
  vector<shared_ptr<Polygon>> front;
  vector<shared_ptr<Polygon>> back;

  if (polygons.size() == 0)
  {
    return;
  }

  divider = polygons.at(0)->clone();
  for (size_t i{0}; i < polygons.size(); i++)
  {
    divider->splitPolygon(polygons.at(i), this->polygons, this->polygons, front, back);
  }

  if (front.size() > 0)
  {
    this->front = make_shared<Node>(front);
  }

  if (back.size() > 0)
  {
    this->back = make_shared<Node>(back);
  }
}

bool Node::isConvex(vector<shared_ptr<Polygon>> const &polygons)
{
  for (size_t i{0}; i < polygons.size(); i++)
  {
    for (size_t j{0}; j < polygons.size(); j++)
    {
      if (i != j && polygons.at(i)->classifySide(polygons.at(j)) != BACK)
      {
        return false;
      }
    }
  }
  return true;
}

void Node::build(vector<shared_ptr<Polygon>> const &polygons)
{
  if (!(this->divider) && polygons.size() > 0)
  {
    this->divider = polygons.at(0)->clone();
  }

  vector<shared_ptr<Polygon>> front;
  vector<shared_ptr<Polygon>> back;

  auto polygons_count = polygons.size();
  for (size_t i{0}; i < polygons_count; i++)
  {
    this->divider->splitPolygon(polygons.at(i), this->polygons, this->polygons, front, back);
  }

  if (front.size() > 0)
  {
    if (!(this->front))
    {
      this->front = make_shared<Node>();
    }
    this->front->build(front);
  }

  if (back.size() > 0)
  {
    if (!(this->back))
    {
      this->back = make_shared<Node>();
    }
    this->back->build(back);
  }
}

vector<shared_ptr<Polygon>> Node::allPolygons() const
{
  vector<shared_ptr<Polygon>> polygons;
  polygons.insert(polygons.end(), this->polygons.begin(), this->polygons.end());
  if (this->front)
  {
    auto aux = this->front->allPolygons();
    polygons.insert(polygons.end(), aux.begin(), aux.end());
  }
  if (this->back)
  {
    auto aux = this->back->allPolygons();
    polygons.insert(polygons.end(), aux.begin(), aux.end());
  }
  return polygons;
}

shared_ptr<Node> Node::clone() const
{
  auto node = make_shared<Node>();
  node->divider = this->divider->clone();
  std::transform(this->polygons.begin(), this->polygons.end(), std::back_inserter(node->polygons), [](shared_ptr<Polygon> const &polygon)
                 { return polygon->clone(); });

  node->front = this->front ? this->front->clone() : nullptr;
  node->back = this->back ? this->back->clone() : nullptr;

  return node;
}

shared_ptr<Node> Node::invert()
{
  for (auto &polygon : this->polygons)
  {
    polygon->flip();
  }
  this->divider->flip();
  if (this->front)
  {
    this->front->invert();
  }
  if (this->back)
  {
    this->back->invert();
  }
  auto temp = this->front;
  this->front = this->back;
  this->back = temp;

  return shared_from_this();
}

vector<shared_ptr<Polygon>> Node::clipPolygons(vector<shared_ptr<Polygon>> const &polygons) const
{

  if (!(this->divider))
  {
    return polygons;
  }

  vector<shared_ptr<Polygon>> front;
  vector<shared_ptr<Polygon>> back;

  size_t polygon_count = polygons.size();
  for (size_t i{0}; i < polygon_count; i++)
  {
    this->divider->splitPolygon(polygons.at(i), front, back, front, back);
  }

  if (this->front)
  {
    front = this->front->clipPolygons(front);
  }

  if (this->back)
  {
    back = this->back->clipPolygons(back);
  }
  else
  {
    back.clear();
  }

  front.insert(front.end(), back.begin(), back.end());
  return front;
}

void Node::clipTo(shared_ptr<Node> const &node)
{
  this->polygons = node->clipPolygons(this->polygons);
  if (this->front)
  {
    this->front->clipTo(node);
  }
  if (this->back)
  {
    this->back->clipTo(node);
  }
}