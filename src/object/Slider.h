#pragma once
#include "Sprite.h"

namespace rhythmus
{
  
class LR2FnArgs;

class Slider : public BaseObject
{
public:
  Slider();
  virtual ~Slider();
  virtual void SetNumber(int number);
  virtual void SetNumber(double number);
  virtual void Refresh();

  virtual void Load(const MetricGroup &metric);
  virtual void RunLR2Command(const std::string& command, const LR2FnArgs& args);

  void SetRange(int direction, float range);
  void SetResource(const std::string &resourcename);
  void SetEditable(bool editable);
  Sprite *cursor();

  virtual const char* type() const;
  virtual std::string toString() const;

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

  virtual void OnAnimation(DrawProperty &frame);
  virtual void doUpdate(double);
  virtual void doRender();
};

}
