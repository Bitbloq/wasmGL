#include "color.h"

Color::Color(float r, float g, float b, float a)
{
  setRGB(r, g, b, a);
}

Color::Color(string hex, float a)
{
  setHex(hex, a);
}

void Color::setRGB(float r, float g, float b, float a)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}
void Color::setHex(string const &hex, float a)
{
  int r, g, b;
  sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
  this->r = r / 255.0;
  this->g = g / 255.0;
  this->b = b / 255.0;
  this->a = a;
}
string Color::toString() const
{
  return "#" + to_string((int)(r * 255)) + to_string((int)(g * 255)) + to_string((int)(b * 255));
}