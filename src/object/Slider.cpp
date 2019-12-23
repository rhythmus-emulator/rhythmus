#include "Slider.h"
#include "Script.h"
#include "Util.h"

namespace rhythmus
{

Slider::Slider()
  : direction_(0), range_(0), ratio_(1.0f), value_(.0f) { }

Slider::~Slider() { }

void Slider::SetNumber(int number)
{
  value_ = number * ratio_;
}

void Slider::SetNumber(double number)
{
  value_ = number * ratio_;
}

void Slider::Refresh()
{
  // XXX: change to GetSliderValue, or value with "Namespace"
  SetNumber( Script::getInstance().GetNumber(GetResourceId()) );
}

void Slider::SetRatio(float ratio)
{
  ratio_ = ratio;
}

void Slider::Load(const Metric &metric)
{
  Sprite::Load(metric);
}

void Slider::LoadFromLR2SRC(const std::string &cmd)
{
  Sprite::LoadFromLR2SRC(cmd);
  
  /* clear out OP code */
  SetVisibleGroup();

  CommandArgs args(cmd);

  direction_ = args.Get<int>(9);
  range_ = args.Get<int>(10);

  /* track change of text table */
  int eventid = args.Get<int>(11);
  std::string eventname = "Slider" + args.Get<std::string>(1);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);

  /* set text index for update */
  SetResourceId(eventid);
  Refresh();
}

void Slider::doUpdate(float delta)
{
  Sprite::doUpdate(delta);

  int pos_delta = range_ * value_;
  switch (direction_)
  {
  case 0:
    current_prop_.y -= pos_delta;
    break;
  case 1:
    current_prop_.x += pos_delta;
    break;
  case 2:
    current_prop_.y += pos_delta;
    break;
  case 3:
    current_prop_.x -= pos_delta;
    break;
  }
}

void Slider::doRender()
{
  Sprite::doRender();
}

}