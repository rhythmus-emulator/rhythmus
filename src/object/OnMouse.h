#include "Sprite.h"

namespace rhythmus
{

/* @brief special object for LR2. */
class OnMouse : public Sprite
{
public:
  OnMouse();
  virtual ~OnMouse();
  virtual void Load(const Metric &metric);
  virtual void LoadFromLR2SRC(const std::string &cmd);
  virtual bool IsEntered(float x, float y);
private:
  // XXX: for debug?
  int panel_;
  Rect onmouse_rect_;

  virtual void doRender();
};

}