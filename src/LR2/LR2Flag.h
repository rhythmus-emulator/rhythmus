#pragma once

#include "Timer.h"

namespace rhythmus
{

namespace LR2Flag
{
  int GetFlag(int flag_no);
  const char* GetText(int text_no);
  bool IsTimerActive(int timer_no);
  void Update();
  bool ShouldActiveTimer(int timer_no);

  void SubscribeEvent();
}

/* @brief common attributes for LR2 objects */
class LR2BaseObject
{
public:
  LR2BaseObject();
  bool IsLR2Visible() const;
  void set_op(int op1, int op2 = 0, int op3 = 0);
  void set_timer_id(int timer_id);
private:
  int op_[3];
  int timer_id_;
};

}