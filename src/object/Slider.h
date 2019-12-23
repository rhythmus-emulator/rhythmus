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

  virtual void Load(const Metric &metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);

private:
  // 0: horizontal, 1: right, 2: bottom, 3: left
  int direction_;
  int range_;
  float ratio_;
  float value_;

  virtual void doUpdate(float delta);
  virtual void doRender();
};

}