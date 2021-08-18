#include "Slider.h"
#include "ScriptLR2.h"
#include "KeyPool.h"
#include "Util.h"
#include "Logger.h"
#include "config.h"
#include <sstream>
#include <algorithm>
#include <memory.h>

namespace rhythmus
{

Slider::Slider()
  : type_(0), maxvalue_(1.0f), value_(.0f),
    val_ptr_(nullptr), editable_(false)
{
  memset(&range_, 0, sizeof(Vector2));
  set_xy_as_center_ = false;
}

Slider::~Slider() { }

void Slider::SetNumber(int number)
{
  SetNumber(number / 100.0f);
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
  cursor_.SetDraggable(true);
  AddStaticChild(&cursor_);

  BaseObject::Load(metric);

  metric.get_safe("direction", type_);

  if (metric.exist("path"))
    cursor_.SetImage(metric.get_str("path"));
}

void Slider::SetRange(int direction, float range)
{
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
}

void Slider::SetResource(const std::string &resname)
{
  auto k = KEYPOOL->GetFloat(resname);
  val_ptr_ = &*k;
}

void Slider::SetEditable(bool editable)
{
  editable_ = editable;
}

Sprite *Slider::cursor()
{
  return &cursor_;
}

void Slider::OnAnimation(DrawProperty &frame)
{
  // We need to modify 'slider size'
  // to receive event in case of LR2 object,
  // as it's slider object drawing property is different from
  // actual size of slider.

  int type = (type_ & 0xF);
  int use_range_override = (type_ & 0xF0);
  if (use_range_override)
  {
    // TODO: also cursor size reset when OnResize()
    cursor_.SetWidth(rhythmus::GetWidth(frame.pos));
    cursor_.SetHeight(rhythmus::GetHeight(frame.pos));

    if (type == 0)
      frame.pos.w = frame.pos.y + range_.y;
    else if (type == 1)
      frame.pos.z = frame.pos.x + range_.x;
  }
}

void Slider::doUpdate(double)
{
  int type = (type_ & 0xF);
  int use_range_override = (type_ & 0xF0);
  auto &f = GetCurrentFrame();

  // Update updated value if editable.
  if (editable_ && cursor_.IsDragging())
  {
    value_ = cursor_.GetY() / rhythmus::GetHeight(f.pos);
    value_ = std::max(std::min(1.0f, value_), 0.0f);
  }

  // Set position of the sprite cursor.
  switch (type)
  {
  case 0:
    cursor_.SetX(0);
    cursor_.SetY(rhythmus::GetHeight(f.pos) * value_);
    break;
  case 2:
    cursor_.SetX(0);
    cursor_.SetY(rhythmus::GetHeight(f.pos) * (1.0f - value_));
    break;
  case 1:
    cursor_.SetX(rhythmus::GetWidth(f.pos) * (1.0f - value_));
    cursor_.SetY(0);
    break;
  case 3:
    cursor_.SetX(rhythmus::GetWidth(f.pos) * value_);
    cursor_.SetY(0);
    break;
  }

  // in case of range override
  // TODO: only update actual size?
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
}

void Slider::doRender()
{
}

const char* Slider::type() const { return "Slider"; }

std::string Slider::toString() const
{
  std::stringstream ss;
  ss << "type: " << type_ << std::endl;
  ss << "maxvalue: " << maxvalue_ << std::endl;
  ss << "value: " << value_ << std::endl;
  if (val_ptr_)
    ss << "val: " << *val_ptr_ << std::endl;
  else
    ss << "(val empty)" << std::endl;
  ss << "editable: " << editable_ << std::endl;
  ss << "\nCURSOR DESC" << std::endl << cursor_.toString();
  return BaseObject::toString() + ss.str();
}


#define HANDLERLR2_OBJNAME Slider
REGISTER_LR2OBJECT(Slider);

class SliderLR2Handler : public LR2FnClass {
public:
  HANDLERLR2(SRC_SLIDER) {
    // TODO: sprite 'src' attribute
    /* (null),imgname,sx,sy,sw,sh,divx,divy,cycle,timer */

    // only load texture path & texture coord for cursor
    // TODO: set cycle for image
    o->cursor()->SetImage(std::string("image") + args.get_str(2));
    o->cursor()->SetImageCoord(Rect{
      args.get_float(3), args.get_float(4),
      args.get_float(3) + args.get_float(5),
      args.get_float(4) + args.get_float(6) });

    int direction = args.get_int(11);
    float range = args.get_float(12);
    o->SetRange(direction, range);

    /* track change of text table */
    int eventid = args.get_int(13) + 1500;
    std::string eventname = "Number" + std::to_string(eventid);
    o->AddCommand(eventname, "refresh");
    o->SubscribeTo(eventname);

    /* set ref value for update */
    o->SetResource("slider" + std::to_string(eventid));
    o->Refresh();

    /* disabled? */
    o->SetEditable(args.get_int(14) == 0);
  }
  HANDLERLR2(DST_SLIDER) {
    // these attributes are only affective for first run
    if (o->GetAnimation().is_empty()) {
      const int timer = args.get_int(17);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      //o->SetBlending(ctx->get_int(12));
      //o->SetFiltering(ctx->get_int(13));

      // XXX: move this flag to Sprite,
      // as LR2_TEXT object don't work with this..?
      o->SetVisibleFlag(
        format_string("F%s", args.get_str(18)),
        format_string("F%s", args.get_str(19)),
        format_string("F%s", args.get_str(20)),
        std::string()
      );
    }
  }
  SliderLR2Handler() : LR2FnClass(
    GetTypename<Slider>(), GetTypename<BaseObject>()
  ) {
    ADDSHANDLERLR2(SRC_SLIDER);
    ADDSHANDLERLR2(DST_SLIDER);
  }
};

SliderLR2Handler _SliderLR2Handler;

}
