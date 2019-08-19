#include "LR2Flag.h"
#include "Game.h"

namespace rhythmus
{

  namespace LR2Flag
  {
    // DST OP Flags used by LR2.
    static int LR2Flag[1000];

    // previously stored timer status.
    bool LR2Timer_prev_activated[1000];

    // current timer status.
    bool LR2Timer_curr_activated[1000];

    int GetFlag(int flag_no)
    {
      if (flag_no >= 0)
        return LR2Flag[flag_no];
      else
        return LR2Flag[-flag_no] > 0 ? 1 : 0;
    }

    void Update()
    {
      // TODO: update LR2Flag.
      memset(LR2Flag, 0, sizeof(LR2Flag));
      LR2Flag[0] = 1;

      memcpy(LR2Timer_prev_activated, LR2Timer_curr_activated, sizeof(LR2Timer_prev_activated));

      // TODO: update LR2Timer.
      memset(LR2Timer_curr_activated, 0, sizeof(LR2Timer_curr_activated));
      LR2Timer_curr_activated[0] = true;
    }

    bool ShouldActiveTimer(int timer_no)
    {
      // TODO: more detailed checking necessary
      // e.g. checking timer start time
      return LR2Timer_curr_activated[timer_no];
    }

    bool IsTimerActive(int timer_no)
    {
      return LR2Timer_curr_activated[timer_no];
    }

  }

}