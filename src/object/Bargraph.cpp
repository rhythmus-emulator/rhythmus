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

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVBargraphHandlers
{
public:
  static void src_bargraph(Bargraph *&o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    o->SetDirection(ctx->get_int(9));
    std::string resname = "bargraph";
    resname += ctx->get_str(11);
    o->SetResourceId(resname);

    // Load SRC information to bar_ Sprite.
    LR2CSVExecutor::CallHandler("#SRC_IMAGE", o->sprite(), loader, ctx);
    o->sprite()->DeleteAllCommand();
  }
  LR2CSVBargraphHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BARGRAPH", (LR2CSVHandlerFunc)&src_bargraph);
    LR2CSVExecutor::AddTrigger("#SRC_BARGRAPH", "#SRC_BASE_");
    LR2CSVExecutor::AddTrigger("#SRC_BARGRAPH", "#DST_BASE_");
  }
};

// register handler
LR2CSVBargraphHandlers _LR2CSVBargraphHandlers;

}
