#include "Bargraph.h"
#include "KeyPool.h"
#include "Util.h"
#include "Script.h"
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

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVBargraphHandlers
{
public:
  static void src_bargraph(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *o = _this ? (Bargraph*)_this : (Bargraph*)BaseObject::CreateObject("bargraph");
    loader->set_object("bargraph", o);
    // TODO: use general SRC property later to bar sprite
    //LR2CSVExecutor::CallHandler("#SRC_IMAGE", o, loader, ctx);

    o->SetDirection(ctx->get_int(9));
    std::string resname = "bargraph";
    resname += ctx->get_str(11);
    o->SetResourceId(resname);
  }
  static void dst_bargraph(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *o = _this ? (Bargraph*)_this : (Bargraph*)loader->get_object("bargraph");
    LR2CSVExecutor::CallHandler("#DST_IMAGE", o, loader, ctx);
  }
  LR2CSVBargraphHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BARGRAPH", (LR2CSVCommandHandler)&src_bargraph);
    LR2CSVExecutor::AddHandler("#DST_BARGRAPH", (LR2CSVCommandHandler)&dst_bargraph);
  }
};

// register handler
LR2CSVBargraphHandlers _LR2CSVBargraphHandlers;

}
