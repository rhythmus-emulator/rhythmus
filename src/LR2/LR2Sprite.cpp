#include "LR2Sprite.h"
#include "Timer.h"

namespace rhythmus
{

// ---------------------------- class LR2Sprite

LR2Sprite::LR2Sprite()
  : loop_(0), src_timer_id_(0), attr_loaded_(false)
{
  set_name("LR2Sprite");
}

void LR2Sprite::LoadProperty(const std::string& prop_name, const std::string& value)
{
  Sprite::LoadProperty(prop_name, value);

  if (prop_name == "#SRC_IMAGE")
  {
    // use SRC timer here
    src_timer_id_ = GetAttribute<int>("src_timer");
  }
  else if (prop_name == "#DST_IMAGE")
  {
    if (!attr_loaded_)
    {
      set_op(GetAttribute<int>("op0"),
        GetAttribute<int>("op1"),
        GetAttribute<int>("op2"));
      set_timer_id(GetAttribute<int>("timer"));
      loop_ = GetAttribute<int>("loop");
      attr_loaded_ = true;
    }
    else SetTweenLoopTime(loop_); /* a bit ineffcient but works well anyway. */
  }
}

bool LR2Sprite::IsVisible() const
{
  return IsLR2Visible() && Sprite::IsVisible();
}

}