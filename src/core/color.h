#ifndef COLOR_H
#define COLOR_H

#include <string>

using namespace std;

class Color
{
public:
  Color(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

  Color(string hex = "#000000", float a = 1.0f);

  void setRGB(float r, float g, float b, float a = 1.0f);
  void setHex(string const &hex, float a = 1.0f);
  string toString() const;

  float r, g, b, a;
};

#endif