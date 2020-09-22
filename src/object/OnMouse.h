#include "Sprite.h"

namespace rhythmus
{

/* @brief special object for LR2. */
class OnMouse : public Sprite
{
public:
  OnMouse();
  virtual ~OnMouse();
  virtual void Load(const MetricGroup &metric);
  virtual bool IsEntered(float x, float y);
  virtual const char* type() const;

  void SetOnmouseRect(const Rect& r);

private:
  Rect onmouse_rect_;

  virtual void doRender();
};

}