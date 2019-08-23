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

}