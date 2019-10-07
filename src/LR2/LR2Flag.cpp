#include "LR2Flag.h"
#include "Event.h"
#include "Game.h"
#include "Song.h"
#include "SceneManager.h"
#include "rutil.h"  /* string modification */
#include "Error.h"

namespace rhythmus
{

  namespace LR2Flag
  {
    // DST OP Flags used by LR2.
    static int LR2Flag[1000];

    // Timer started time. (Zero : not started)
    uint32_t LR2Timer[1000];

    // strings for TEXT
    std::string LR2Text[1000];

    int GetFlag(int flag_no)
    {
      ASSERT(flag_no < 1000);
      if (flag_no >= 0)
        return LR2Flag[flag_no];
      else
        return LR2Flag[-flag_no] == 0 ? 1 : 0;
    }

    const char* GetText(int text_no)
    {
      ASSERT(text_no < 1000);
      return LR2Text[text_no].c_str();
    }

    bool IsTimerActive(size_t timer_no)
    {
      return timer_no == 0 || LR2Timer[timer_no] > 0;
    }

    void ActiveTimer(size_t timer_no)
    {
      LR2Timer[timer_no] = Timer::GetGameTimeInMillisecond();
    }

    void DeactiveTimer(size_t timer_no)
    {
      LR2Timer[timer_no] = 0;
    }

    uint32_t GetTimerTime(size_t timer_no)
    {
      return (timer_no > 0 && LR2Timer[timer_no] == 0) ?
        0 : Timer::GetGameTimeInMillisecond() - LR2Timer[timer_no];
    }

    void Update()
    {
      // TODO: update LR2Flag.
      memset(LR2Flag, 0, sizeof(LR2Flag));
      LR2Flag[0] = 1;
      LR2Flag[50] = 1;  // OFFLINE
      LR2Flag[52] = 1;  // EXTRA MODE OFF
    }



    class LR2EventReceiver : public EventReceiver
    {
      virtual bool OnEvent(const EventMessage &e)
      {
        switch (e.GetEventID())
        {
          case Events::kEventSceneConfigLoaded:
          {
            // update LR2flag status
            for (int i = 900; i < 1000; ++i)
              LR2Flag[i] = 0;
            const auto& theme_param = SceneManager::get_current_scene()->get_theme_parameter();
            for (auto ii : theme_param.attributes)
            {
              int flag_no = atoi(ii.first.c_str());
              if (flag_no >= 900 && flag_no < 1000)
                LR2Flag[flag_no] = 1;
            }
            break;
          }
          case Events::kEventSongSelectChanged:
          {
            auto &sel_data = *SongList::getInstance().get_current_song_info();
            LR2Text[10] = sel_data.title;
            LR2Text[11] = sel_data.subtitle;
            LR2Text[12] = sel_data.title + " " + sel_data.subtitle;
            LR2Text[13] = sel_data.genre;
            LR2Text[14] = sel_data.artist;
            LR2Text[15] = sel_data.subartist;
            break;
          }
          case Events::kEventPlayLoading:
            DeactiveTimer(40);  // READY
            DeactiveTimer(41);  // START
            break;
          case Events::kEventPlayReady:
            ActiveTimer(40);
            break;
          case Events::kEventPlayStart:
            ActiveTimer(41);
            break;
        }
        return true;
      }
    };

    void SubscribeEvent()
    {
      static LR2EventReceiver r;
      r.SubscribeTo(Events::kEventSceneConfigLoaded);
      r.SubscribeTo(Events::kEventSongSelectChanged);
      r.SubscribeTo(Events::kEventPlayLoading);
      r.SubscribeTo(Events::kEventPlayReady);
      r.SubscribeTo(Events::kEventPlayStart);
    }
  }

  /* class LR2BaseObject */

  LR2BaseObject::LR2BaseObject()
    : timer_id_(0) { memset(op_, 0, sizeof(op_)); }

  bool LR2BaseObject::IsLR2Visible() const
  {
    return LR2Flag::GetFlag(op_[0]) && LR2Flag::GetFlag(op_[1]) &&
           LR2Flag::GetFlag(op_[2]) && LR2Flag::IsTimerActive(timer_id_);
  }

  void LR2BaseObject::set_op(int op1, int op2, int op3)
  {
    op_[0] = op1;
    op_[1] = op2;
    op_[2] = op3;
  }

  void LR2BaseObject::set_timer_id(int timer_id)
  {
    timer_id_ = timer_id;
  }
}