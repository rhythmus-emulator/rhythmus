#include "Slider.h"
#include "Script.h"
#include "KeyPool.h"
#include "Util.h"
#include "config.h"

namespace rhythmus
{

Slider::Slider()
  : maxvalue_(1.0f), value_(.0f), ref_ptr_(nullptr)
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
  if (ref_ptr_)
    SetNumber( *ref_ptr_);
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

    int direction = args.Get<int>(10);
    int range = args.Get<int>(11);
    switch (direction)
    {
    case 0:
    default:
      range_.y -= (float)range;
      break;
    case 1:
      range_.x += (float)range;
      break;
    case 2:
      range_.y += (float)range;
      break;
    case 3:
      range_.x -= (float)range;
      break;
    }

    /* track change of text table */
    int eventid = args.Get<int>(12) + 1500;
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
}

void Slider::doRender()
{
  // translate and render
  Vector3 tv(range_.x * value_, range_.y * value_, 0.0f);
  GRAPHIC->Translate(tv);

  Sprite::doRender();
}

}
