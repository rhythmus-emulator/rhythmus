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

void OnMouse::Load(const MetricGroup &metric)
{
  Sprite::Load(metric);
}

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

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVOnMouseHandlers
{
public:
  static void src_onmouse(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *o = _this ? (OnMouse*)_this : (OnMouse*)BaseObject::CreateObject("onmouse");
    loader->set_object("onmouse", o);
    LR2CSVExecutor::CallHandler("#SRC_IMAGE", o, loader, ctx);

    int panel = ctx->get_int(10);
    if (panel > 0 || panel == -1)
    {
      if (panel == -1) panel = 0;
      o->AddCommand("Panel" + std::to_string(panel), "focusable:1");
      o->AddCommand("Panel" + std::to_string(panel) + "Off", "focusable:0");
      o->SetVisibleFlag("", "", "", std::to_string(20 + panel));
    }

    Rect r = Vector4(ctx->get_int(11), ctx->get_int(12),
      ctx->get_int(13), ctx->get_int(14));
    o->SetOnmouseRect(r);
  }
  static void dst_onmouse(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *o = _this ? (OnMouse*)_this : loader->get_object("onmouse");
    LR2CSVExecutor::CallHandler("#DST_IMAGE", o, loader, ctx);
  }
  LR2CSVOnMouseHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_ONMOUSE", (LR2CSVCommandHandler)&src_onmouse);
    LR2CSVExecutor::AddHandler("#DST_ONMOUSE", (LR2CSVCommandHandler)&dst_onmouse);
  }
};

// register handler
LR2CSVOnMouseHandlers _LR2CSVOnMouseHandlers;

}
