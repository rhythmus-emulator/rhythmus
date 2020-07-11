#include "OnMouse.h"
#include "Util.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

OnMouse::OnMouse() : panel_(0)
{
  memset(&onmouse_rect_, 0, sizeof(Rect));
}

OnMouse::~OnMouse() {}

void OnMouse::Load(const MetricGroup &metric)
{
  Sprite::Load(metric);

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    std::string cmd;
    metric.get_safe("lr2src", cmd);
    CommandArgs args(cmd);

    panel_ = args.Get<int>(9);
    if (panel_ > 0 || panel_ == -1)
    {
      if (panel_ == -1) panel_ = 0;
      AddCommand("Panel" + std::to_string(panel_), "focusable:1");
      AddCommand("Panel" + std::to_string(panel_) + "Off", "focusable:0");
      SetVisibleFlag("", "", "", std::to_string(20 + panel_));
    }

    onmouse_rect_ = Vector4(args.Get<int>(10), args.Get<int>(11),
      args.Get<int>(12), args.Get<int>(13));
  }
#endif
}

bool OnMouse::IsEntered(float x, float y)
{
  auto &f = GetCurrentFrame();
  x -= f.pos.x + onmouse_rect_.x;
  y -= f.pos.y + onmouse_rect_.y;
  return (x >= 0 && x <= GetWidth(onmouse_rect_)
    && y >= 0 && y <= GetHeight(onmouse_rect_));
}

void OnMouse::doRender()
{
  if (!is_hovered_)
    return;
  Sprite::doRender();
}

const char* OnMouse::type() const { return "OnMouse"; }

}
