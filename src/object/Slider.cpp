#include "Slider.h"
#include "Script.h"
#include "KeyPool.h"
#include "Util.h"
#include "config.h"
#include <memory.h>

namespace rhythmus
{

Slider::Slider()
  : type_(0), maxvalue_(1.0f), value_(.0f),
    val_ptr_(nullptr), editable_(false)
{
  memset(&range_, 0, sizeof(Vector2));
}

Slider::~Slider() { }

void Slider::SetNumber(int number)
{
  value_ = number / 100.0f;
}

void Slider::SetNumber(double number)
{
  value_ = (float)number;
}

void Slider::Refresh()
{
  if (val_ptr_)
    SetNumber( *val_ptr_);
}

void Slider::Load(const MetricGroup &metric)
{
  SetOwnChildren(false);
  AddChild(&cursor_);
  BaseObject::Load(metric);

  cursor_.Load(metric);

  metric.get_safe("direction", type_);

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    std::string cmd;
    metric.get_safe("lr2src", cmd);
    CommandArgs args(cmd);

    int direction = args.Get<int>(10);
    int range = args.Get<int>(11);
    switch (direction)
    {
    case 0:
    default:
      range_.y -= (float)range;
      type_ = 0x10;
      break;
    case 1:
      range_.x += (float)range;
      type_ = 0x11;
      break;
    case 2:
      range_.y += (float)range;
      type_ = 0x10;
      break;
    case 3:
      range_.x -= (float)range;
      type_ = 0x11;
      break;
    }

    /* track change of text table */
    int eventid = args.Get<int>(12) + 1500;
    std::string eventname = "Number" + std::to_string(eventid);
    AddCommand(eventname, "refresh");
    SubscribeTo(eventname);

    /* set ref value for update */
    auto k = KEYPOOL->GetFloat("slider" + std::to_string(eventid));
    val_ptr_ = &*k;
    Refresh();

    /* disabled? */
    if (args.Get_str(13))
      editable_ = (args.Get<int>(13) == 0);
  }
#endif
}

void Slider::doUpdateAfter()
{
  int type = (type_ & 0xF);
  int use_range_override = (type_ & 0xF0);
  auto f = GetCurrentFrame();

  // in case of range override 
  if (use_range_override)
  {
    switch (type)
    {
    case 0:
    case 2:
      f.pos.w = f.pos.y + range_.y;
      break;
    case 1:
    case 3:
      f.pos.z = f.pos.x + range_.x;
      break;
    }
  }

  // Set position of the sprite cursor
  // Must use doUpdateAfter() since slider position should be set
  // after animation.
  switch (type)
  {
  case 0:
    cursor_.SetX(0);
    cursor_.SetY(GetHeight(f.pos) * value_);
    break;
  case 2:
    cursor_.SetX(0);
    cursor_.SetY(GetHeight(f.pos) * (1.0f - value_));
    break;
  case 1:
    cursor_.SetX(GetWidth(f.pos) * (1.0f - value_));
    cursor_.SetY(0);
    break;
  case 3:
    cursor_.SetX(GetWidth(f.pos) * value_);
    cursor_.SetY(0);
    break;
  }
}

void Slider::doRender()
{
}

void Slider::UpdateRenderingSize(Vector2 &d, Vector3 &p)
{
  // We need to modify 'slider size'
  // to receive event in case of LR2 object,
  // as it's slider object drawing property is different from
  // actual size of slider.

  int type = (type_ & 0xF);
  int use_range_override = (type_ & 0xF0);
  if (use_range_override)
  {
    if (type == 0)
      d.y = range_.y;
    else if (type == 1)
      d.x = range_.x;
  }
}

}
