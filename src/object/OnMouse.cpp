#include "OnMouse.h"
#include "Util.h"
#include "ScriptLR2.h"
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


#define HANDLERLR2_OBJNAME OnMouse
REGISTER_LR2OBJECT(OnMouse);

class OnMouseLR2Handler : public LR2FnClass {
public:
  HANDLERLR2(SRC_ONMOUSE) {
    int panel = args.get_int(10);
    if (panel > 0 || panel == -1) {
      if (panel == -1) panel = 0;
      o->AddCommand("Panel" + std::to_string(panel), "focusable:1");
      o->AddCommand("Panel" + std::to_string(panel) + "Off", "focusable:0");
      o->SetVisibleFlag("", "", "", std::to_string(20 + panel));
    }

    Rect r = Vector4(args.get_int(11), args.get_int(12),
      args.get_int(13), args.get_int(14));
    o->SetOnmouseRect(r);
  }
  HANDLERLR2(DST_ONMOUSE) {
  }
  OnMouseLR2Handler() : LR2FnClass(
    GetTypename<OnMouse>(), GetTypename<Sprite>()
  ) {
    ADDSHANDLERLR2(SRC_ONMOUSE);
    ADDSHANDLERLR2(DST_ONMOUSE);
  }
};

OnMouseLR2Handler _OnMouseLR2Handler;

}
