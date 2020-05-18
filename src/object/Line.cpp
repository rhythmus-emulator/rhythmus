#include "Line.h"

namespace rhythmus
{

Line::Line() : width_(1.0f)
{
  color_.a = color_.r = color_.g = color_.b = 0xFF;
}

Line::~Line() { }

void Line::SetColor(uint32_t color)
{
  SetColor(
    static_cast<uint8_t>(color >> 24),
    static_cast<uint8_t>(color >> 16),
    static_cast<uint8_t>(color >> 8),
    static_cast<uint8_t>(color)
  );
}

void Line::SetColor(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
  color_.r = r;
  color_.g = g;
  color_.b = b;
  color_.a = a;
}

void Line::SetColor(uint8_t r, uint8_t g, uint8_t b)
{
  color_.r = r;
  color_.g = g;
  color_.b = b;
}

void Line::SetWidth(float width)
{
  width_ = width;
}

void Line::doUpdate(float delta) { }

void Line::doRender()
{
  // XXX: is this even work?
  GRAPHIC->SetTexture(0, 0);
  GRAPHIC->SetBlendMode(1);
#if 0
  glLineWidth(width_);
  glColor4ui(color_.r, color_.g, color_.b, color_.a);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
  glFlush();
#endif
}

}