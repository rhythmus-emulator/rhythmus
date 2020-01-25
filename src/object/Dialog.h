#pragma once
#include "BaseObject.h"
#include "Sprite.h"
#include "Text.h"

namespace rhythmus
{

class Dialog : public BaseObject
{
public:
  Dialog();
  virtual ~Dialog();
  virtual void Load(const Metric &metric);
  void SetTitle(const std::string &text);
  void SetText(const std::string &text);

private:
  /* TL/TC/TR/L/C/R/BL/BC/BR */
  Sprite sprites_[9];
  Rect src_rect_[9];

  /* Text */
  Text title_, text_;

  virtual void doUpdate(float delta);
};

}