
/******************************************************************************
* MODULE     : true_color.hpp
* DESCRIPTION: RGBA colors represented by floating point numbers
* COPYRIGHT  : (C) 2013  Joris van der Hoeven
*******************************************************************************
* This software falls under the GNU general public license version 3 or later.
* It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
* in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
******************************************************************************/

#ifndef TRUE_COLOR_H
#define TRUE_COLOR_H
#include "tree.hpp"

typedef unsigned int color;

class true_color {
public:
  float b;
  float g;
  float r;
  float a;

  inline true_color () {}
  inline true_color (const true_color& c):
    b (c.b), g (c.g), r (c.r), a (c.a) {}
  inline true_color (float r2, float g2, float b2, float a2):
    b (b2), g (g2), r (r2), a (a2) {}
  inline true_color (color c):
    b (((float) (c & 0xff)) / 255.0),
    g (((float) ((c >> 8) & 0xff)) / 255.0),
    r (((float) ((c >> 16) & 0xff)) / 255.0),
    a (((float) ((c >> 24) & 0xff)) / 255.0) {}
  inline operator color () {
    return
      ((int) (b * 255 + 0.5)) +
      (((int) (g * 255 + 0.5)) << 8) +
      (((int) (r * 255 + 0.5)) << 16) +
      (((int) (a * 255 + 0.5)) << 24); }
};

inline tm_ostream&
operator << (tm_ostream& out, const true_color& c) {
  return out << "[ " << c.r << ", " << c.g << ", " << c.b
             << "; " << c.a << "]";
}

inline float
get_alpha (const true_color& c) {
  return c.a;
}

inline true_color
normalize (const true_color& c) {
  return true_color (max (min (c.r, 1.0), 0.0),
                     max (min (c.g, 1.0), 0.0),
                     max (min (c.b, 1.0), 0.0),
                     max (min (c.a, 1.0), 0.0));
}

inline true_color
operator + (const true_color& c1, const true_color& c2) {
  return true_color (c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline true_color
operator - (const true_color& c1, const true_color& c2) {
  return true_color (c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline true_color&
operator += (true_color& c1, const true_color& c2) {
  c1.r += c2.r; c1.g += c2.g; c1.b += c2.b; c1.a += c2.a;
  return c1;
}

inline true_color&
operator -= (true_color& c1, const true_color& c2) {
  c1.r -= c2.r; c1.g -= c2.g; c1.b -= c2.b; c1.a -= c2.a;
  return c1;
}

inline true_color
operator * (float x, const true_color& c) {
  return true_color (c.r * x, c.g * x, c.b * x, c.a * x);
}

inline true_color
operator * (const true_color& c, float x) {
  return true_color (c.r * x, c.g * x, c.b * x, c.a * x);
}

inline true_color
operator / (const true_color& c, float x) {
  return true_color (c.r / x, c.g / x, c.b / x, c.a / x);
}

inline true_color
hide_alpha (const true_color& c) {
  return true_color (c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

inline true_color
show_alpha (const true_color& c) {
  if (c.a < 0.00390625 && c.a > -0.00390625) return c;
  else return true_color (c.r / c.a, c.g / c.a, c.b / c.a, c.a);
}

inline void
source_over (true_color& c1, const true_color& c2) {
  float a1= c1.a, a2= c2.a, a= a2 + a1 * (1 - a2);
  float u= 1.0 / (a + 1.0e-6);
  float f1= a1 * (1 - a2) * u, f2= a2 * u;
  c1.r= c1.r * f1 + c2.r * f2;
  c1.g= c1.g * f1 + c2.g * f2;
  c1.b= c1.b * f1 + c2.b * f2;
  c1.a= a;
}

inline void
towards_source (true_color& c1, const true_color& c2) {
  float a2= c2.a, a1= 1.0 - a2;
  c1.r= c1.r * a1 + c2.r * a2;
  c1.g= c1.g * a1 + c2.g * a2;
  c1.b= c1.b * a1 + c2.b * a2;
}

#endif // defined TRUE_COLOR_H