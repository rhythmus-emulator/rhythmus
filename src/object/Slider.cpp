#include "Slider.h"
#include "Script.h"
#include "KeyPool.h"
#include "Util.h"
#include "config.h"

namespace rhythmus
{

Slider::Slider()
  : direction_(0), range_(0), ratio_(1.0f), value_(.0f), ref_ptr_(nullptr) { }

Slider::~Slider() { }

void Slider::SetNumber(int number)
{
  value_ = (number / 100.0f) * ratio_;
}

void Slider::SetNumber(double number)
{
  value_ = (float)number * ratio_;
}

void Slider::Refresh()
{
  if (ref_ptr_)
    SetNumber( *ref_ptr_);
}

void Slider::SetRatio(float ratio)
{
  ratio_ = ratio;
}

void Slider::Load(const MetricGroup &metric)
{
  Sprite::Load(metric);

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    std::string cmd;
    metric.get_safe("lr2src", cmd);
    CommandArgs args(cmd);

    direction_ = args.Get<int>(9);
    range_ = args.Get<int>(10);

    /* track change of text table */
    int eventid = args.Get<int>(11) + 1500;
    std::string eventname = "Number" + std::to_string(eventid);
    AddCommand(eventname, "refresh");
    SubscribeTo(eventname);

    /* set ref value for update */
    auto k = KEYPOOL->GetFloat("slider" + std::to_string(eventid));
    ref_ptr_ = &*k;
    Refresh();
  }
#endif
}

void Slider::doUpdate(double delta)
{
  Sprite::doUpdate(delta);

  int pos_delta = (int)(range_ * value_);
  switch (direction_)
  {
  case 0:
    SetY((int)GetCurrentFrame().pos.y - pos_delta);
    break;
  case 1:
    SetX((int)GetCurrentFrame().pos.x + pos_delta);
    break;
  case 2:
    SetY((int)GetCurrentFrame().pos.y + pos_delta);
    break;
  case 3:
    SetX((int)GetCurrentFrame().pos.x - pos_delta);
    break;
  }
}

void Slider::doRender()
{
  Sprite::doRender();
}

}
