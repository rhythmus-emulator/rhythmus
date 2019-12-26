#include "Button.h"
#include "Util.h"

namespace rhythmus
{

Button::Button() : panel_(-1) {}

Button::~Button() {}

void Button::Load(const Metric &metric)
{
  Sprite::Load(metric);
}

void Button::LoadFromLR2SRC(const std::string &cmd)
{
  Sprite::LoadFromLR2SRC(cmd);

  CommandArgs args(cmd);

  SetFocusable(args.Get<int>(10));

  /* XXX: change clickable by panel opening */
  panel_ = args.Get<int>(11);
  if (panel_ >= 0)
  {
    AddCommand("Panel" + std::to_string(panel_), "focusable:1");
    AddCommand("Panel" + std::to_string(panel_) + "Off", "focusable:0");
  }

  /**
   * create command to execute when clicked
   * e.g.
   * Click10  : LR2 click event with name 10
   * Click10R : LR2 click event with name 10, reverse.
   */
  std::string minus;
  if (args.size() > 12 && args.Get<int>(12) == -1)
    minus = "R";
  AddCommand("click",
    format_string("sendevent:Click%d%s", args.Get<int>(9), minus.c_str())
  );
}

}