#include "Button.h"
#include "Util.h"

namespace rhythmus
{

Button::Button() : panel_(-1), button_id_(0) {}

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
  button_id_ = args.Get<int>(9);
  if (args.size() > 12 && args.Get<int>(12) == -1)
    minus = "R";
  AddCommand("click",
    format_string("sendevent:Click%d%s", button_id_, minus.c_str())
  );

  /* Set resource id from button id */
  SetResourceId(button_id_ + 1000);
  AddCommand(format_string("Number%d", button_id_ + 1000), "refresh");

  /* Set duration to zero to prevent unexpected sprite animation */
  ani_info_.duration = 0;
}

}