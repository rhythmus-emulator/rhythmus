#pragma once
#include "Sprite.h"

namespace rhythmus
{

class Slider : public BaseObject
{
public:
  Slider();
  virtual ~Slider();
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void Refresh();

  virtual void Load(const MetricGroup &metric);

private:
  // Slider cursor.
  Sprite cursor_;

  // Movement range of the slider (overriding)
  Vector2 range_;

  // movement type
  // 0: vertical
  // 1: horizontal
  // 2: vertical (bottom to top)
  // 3: horizontal (right to left)
  // if 0x10/0x11, then it uses 'range' overriding for slider property
  // (LR2 specific property)
  int type_;

  float maxvalue_;
  float value_;
  float *val_ptr_;

  bool editable_;

  virtual void doUpdateAfter();
  virtual void doRender();
  virtual void UpdateRenderingSize(Vector2 &d, Vector3 &p);
};

}
