#include "Bargraph.h"
#include "KeyPool.h"
#include "Util.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

Bargraph::Bargraph() : direction_(0), value_(0), val_ptr_(nullptr)
{
  set_xy_as_center_ = false;
  AddChild(&bar_);
}

Bargraph::~Bargraph() {}

void Bargraph::Load(const MetricGroup &metric)
{
  BaseObject::Load(metric);

  metric.get_safe("value", value_);

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    std::string cmd;
    metric.get_safe("lr2src", cmd);
    CommandArgs args(cmd);

    direction_ = args.Get<int>(9);

    std::string resname = "bargraph";
    resname += args.Get_str(10);
    KeyData<float> kdata = KEYPOOL->GetFloat(resname);
    val_ptr_ = &*kdata;

    bar_.LoadLR2SRC(cmd);
  }
#endif
}

void Bargraph::doUpdate(double)
{
  // update width / height of bar sprite
  if (direction_ == 0)
  {
    bar_.SetWidth(GetWidth(GetCurrentFrame().pos) * value_);
    bar_.SetHeight(GetHeight(GetCurrentFrame().pos));
  }
  else if (direction_ == 1)
  {
    bar_.SetWidth(GetWidth(GetCurrentFrame().pos));
    bar_.SetHeight(GetHeight(GetCurrentFrame().pos) * value_);
  }
}

void Bargraph::doRender()
{
  // null
}

}
