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
  void SetRatio(float ratio);

  virtual void Load(const MetricGroup &metric);

private:
  // 0: horizontal, 1: right, 2: bottom, 3: left
  int direction_;
  int range_;
  float ratio_;
  float value_;
  float *ref_ptr_;

  virtual void doUpdate(double delta);
  virtual void doRender();
};

}
