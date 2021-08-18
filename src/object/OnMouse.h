#include "Sprite.h"

namespace rhythmus
{

class LR2FnArgs;

/* @brief special object for LR2. */
class OnMouse : public Sprite
{
public:
  OnMouse();
  virtual ~OnMouse();
  using Sprite::RunCommand;
  virtual void RunCommand(const std::string& command, const LR2FnArgs& args);
  virtual bool IsEntered(float x, float y);
  virtual const char* type() const;

  void SetOnmouseRect(const Rect& r);

private:
  Rect onmouse_rect_;

  virtual void doRender();
};

}