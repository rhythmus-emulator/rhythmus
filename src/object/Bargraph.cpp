#include "Bargraph.h"
#include "KeyPool.h"
#include "Util.h"
#include "ScriptLR2.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

Bargraph::Bargraph() : direction_(0), value_(0), val_ptr_(nullptr)
{
  set_xy_as_center_ = false;
  AddStaticChild(&bar_);
}

Bargraph::~Bargraph() {}

void Bargraph::Load(const MetricGroup &metric)
{
  BaseObject::Load(metric);

  metric.get_safe("value", value_);
  metric.get_safe("direction", direction_);
}

void Bargraph::SetResourceId(const std::string &id)
{
  KeyData<float> kdata = KEYPOOL->GetFloat(id);
  val_ptr_ = &*kdata;
}

void Bargraph::SetDirection(int direction)
{
  direction_ = direction;
}

void Bargraph::doUpdate(double)
{
  // update width / height of bar sprite
  if (direction_ == 0)
  {
    bar_.SetWidth(rhythmus::GetWidth(GetCurrentFrame().pos) * value_);
    bar_.SetHeight(rhythmus::GetHeight(GetCurrentFrame().pos));
  }
  else if (direction_ == 1)
  {
    bar_.SetWidth(rhythmus::GetWidth(GetCurrentFrame().pos));
    bar_.SetHeight(rhythmus::GetHeight(GetCurrentFrame().pos) * value_);
  }
}

void Bargraph::doRender()
{
  // null
}

Sprite* Bargraph::sprite() { return &bar_; }


#define HANDLERLR2_OBJNAME Bargraph
REGISTER_LR2OBJECT(Bargraph);

class BargraphLR2Handler : public LR2FnClass {
public:
  HANDLERLR2(SRC_BARGRAPH) {
    o->SetDirection(args.get_int(9));
    std::string resname = "bargraph";
    resname += args.get_str(11);
    o->SetResourceId(resname);
    o->sprite()->RunLR2Command("#SRC_IMAGE", args);
    o->sprite()->DeleteAllCommand();  // XXX
  }
  HANDLERLR2(DST_BARGRAPH) {
  }
  BargraphLR2Handler() : LR2FnClass(
    GetTypename<Bargraph>(), GetTypename<BaseObject>()
  ) {
    ADDSHANDLERLR2(SRC_BARGRAPH);
    ADDSHANDLERLR2(DST_BARGRAPH);
  }
};

BargraphLR2Handler _BargraphLR2Handler;

}
