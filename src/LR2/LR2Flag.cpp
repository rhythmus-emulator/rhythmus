#include "LR2Flag.h"
#include "Event.h"
#include "Game.h"
#include "SceneManager.h"
#include "scene/SelectScene.h"
#include "rutil.h"  /* string modification */
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

    std::string LR2Text_str[1000];

    // strings for TEXT
    const char* LR2Text[1000];

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
          // XXX: Better to check current scene is really SelectScene
          // before do casting.
          SelectScene* scene =
            static_cast<SelectScene*>(SceneManager::getInstance().get_current_scene());
          auto& sel_data = scene->get_wheel().get_selected_data();
          LR2Text[10] = sel_data.title.c_str();
          LR2Text[11] = sel_data.subtitle.c_str();
          LR2Text_str[12] = sel_data.title + " " + sel_data.subtitle;
          LR2Text[12] = LR2Text_str[12].c_str();
          LR2Text[13] = sel_data.genre.c_str();
          LR2Text[14] = sel_data.artist.c_str();
          LR2Text[15] = sel_data.subartist.c_str();
          break;
        }
        }
        return true;
      }
    };

    void SubscribeEvent()
    {
      static LR2EventReceiver r;
      r.SubscribeTo(Events::kEventSceneConfigLoaded);
      r.SubscribeTo(Events::kEventSongSelectChanged);
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