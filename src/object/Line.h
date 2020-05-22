#pragma once
#include "BaseObject.h"

namespace rhythmus
{

class Line : public BaseObject
{
public:
  Line();
  virtual ~Line();

  void SetColor(uint32_t color);
  void SetColor(uint8_t a, uint8_t r, uint8_t g, uint8_t b);
  void SetColor(uint8_t r, uint8_t g, uint8_t b);
  void SetWidth(float width);

private:
  struct {
    uint8_t a, r, g, b;
  } color_;
  float width_;

  virtual void doRender();
};

}
