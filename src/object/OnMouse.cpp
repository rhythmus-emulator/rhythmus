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

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVOnMouseHandlers
{
public:
  static bool src_onmouse(OnMouse *&o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
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

    return true;
  }
  LR2CSVOnMouseHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_ONMOUSE", (LR2CSVHandlerFunc)&src_onmouse);
    LR2CSVExecutor::AddTrigger("#SRC_ONMOUSE", "#SRC_IMAGE");
    LR2CSVExecutor::AddTrigger("#DST_ONMOUSE", "#DST_BASE_");
  }
};

// register handler
LR2CSVOnMouseHandlers _LR2CSVOnMouseHandlers;

}
