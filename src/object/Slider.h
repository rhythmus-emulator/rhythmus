#pragma once
#include "Sprite.h"

namespace rhythmus
{

class Slider : public Sprite
{
public:
  Slider();
  virtual ~Slider();
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void Refresh();

  virtual void Load(const MetricGroup &metric);

private:
  // Movement range of the Slider.
  Vector2 range_;

  float maxvalue_;
  float value_;
  float *ref_ptr_;

  virtual void doUpdate(double delta);
  virtual void doRender();
};

}
