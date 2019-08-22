#include "LR2Flag.h"
#include "Event.h"
#include "Game.h"
#include "SceneManager.h"
#include "scene/SelectScene.h"
#include "Error.h"

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

    // strings for TEXT
    const char* LR2Text[1000];

    int GetFlag(int flag_no)
    {
      ASSERT(flag_no < 1000);
      if (flag_no >= 0)
        return LR2Flag[flag_no];
      else
        return LR2Flag[-flag_no] > 0 ? 1 : 0;
    }

    const char* GetText(int text_no)
    {
      ASSERT(text_no < 1000);
      return LR2Text[text_no];
    }

    bool IsTimerActive(int timer_no)
    {
      return LR2Timer_curr_activated[timer_no];
    }

    void Update()
    {
      // TODO: update LR2Flag.
      memset(LR2Flag, 0, sizeof(LR2Flag));
      LR2Flag[0] = 1;
      LR2Flag[50] = 1;  // OFFLINE
      LR2Flag[52] = 1;  // EXTRA MODE OFF

      memcpy(LR2Timer_prev_activated, LR2Timer_curr_activated, sizeof(LR2Timer_prev_activated));

      // TODO: update LR2Timer (using EventReceiver?)
      memset(LR2Timer_curr_activated, 0, sizeof(LR2Timer_curr_activated));
      LR2Timer_curr_activated[0] = true;
    }

    bool ShouldActiveTimer(int timer_no)
    {
      // TODO: more detailed checking necessary
      // e.g. checking timer start time
      return LR2Timer_curr_activated[timer_no];
    }



    class LR2EventReceiver : public EventReceiver
    {
      virtual void OnEvent(const EventMessage &e)
      {
        switch (e.GetEventID())
        {
        case Events::kEventSongSelectChanged:
          // XXX: Better to check current scene is really SelectScene
          // before do casting.
          LR2Text[10] = static_cast<SelectScene*>(SceneManager::getInstance().get_current_scene())
            ->get_selected_title();
          break;
        }
      }
    };

    void Subscribe()
    {
      static LR2EventReceiver r;
      r.SubscribeTo(Events::kEventSongSelectChanged);
    }
  }

}