#include "OnMouse.h"
#include "Util.h"
#include "Script.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

OnMouse::OnMouse()
{
  memset(&onmouse_rect_, 0, sizeof(Rect));
}

OnMouse::~OnMouse() {}

void OnMouse::SetOnmouseRect(const Rect& r)
{
  onmouse_rect_ = r;
}

bool OnMouse::IsEntered(float x, float y)
{
  auto &f = GetCurrentFrame();
  x -= f.pos.x + onmouse_rect_.x;
  y -= f.pos.y + onmouse_rect_.y;
  return (x >= 0 && x <= rhythmus::GetWidth(onmouse_rect_)
    && y >= 0 && y <= rhythmus::GetHeight(onmouse_rect_));
}

void OnMouse::doRender()
{
  if (!is_hovered_)
    return;
  Sprite::doRender();
}

const char* OnMouse::type() const { return "OnMouse"; }

}
