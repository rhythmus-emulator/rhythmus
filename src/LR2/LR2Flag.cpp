#include "LR2Flag.h"
#include "Event.h"
#include "Game.h"
#include "Song.h"
#include "Script.h"
#include "SceneManager.h"
#include "rutil.h"  /* string modification */
#include "Error.h"

namespace rhythmus
{

  namespace LR2Flag
  {
    // strings for TEXT
    std::string LR2Text[1000];

    const std::string &GetText(size_t text_no)
    {
      ASSERT(text_no < 1000);
      return LR2Text[text_no];
    }

    typedef std::map<std::string, std::function<void()> > EventFnMap;

    /**
     * @brief Hooker that triggers LR2 specified timer & text change events.
     * @warn  Text should be set before send text changing events.
     */
    static EventFnMap &GetEventFnMap()
    {
      static EventFnMap fnmap;
      if (fnmap.empty())
      {
        fnmap["Load"] = []() { EventManager::SendEvent("LR0"); };
        /* Events for SelectScene */
        fnmap["SongSelectChange"] = []() {
          EventManager::SendEvent("LR11");
          EventManager::SendEvent("LR14Off");
        };
        fnmap["SongSelectChangeUp"] = []() {
          EventManager::SendEvent("LR12");
        };
        fnmap["SongSelectChangeDown"] = []() {
          EventManager::SendEvent("LR13");
        };
        fnmap["SongSelectChanged"] = []() {
          EventManager::SendEvent("LR11Off");
          EventManager::SendEvent("LR12Off");
          EventManager::SendEvent("LR13Off");
          EventManager::SendEvent("LR14");
        };
        /* Events for SelectScene Panel */
        fnmap["Click1"] = []() {
          if (Script::getInstance().GetFlag(20) == 1)
          {
            Script::getInstance().SetFlag(20, 0);
            Script::getInstance().SetFlag(21, 1);
            EventManager::SendEvent("LR21");
            if (Script::getInstance().GetFlag(22) == 1)
            {
              Script::getInstance().SetFlag(22, 0);
              EventManager::SendEvent("LR22Off");
              EventManager::SendEvent("LR32");
            }
            if (Script::getInstance().GetFlag(23) == 1)
            {
              Script::getInstance().SetFlag(23, 0);
              EventManager::SendEvent("LR23Off");
              EventManager::SendEvent("LR33");
            }
            if (Script::getInstance().GetFlag(24) == 1)
            {
              Script::getInstance().SetFlag(24, 0);
              EventManager::SendEvent("LR24Off");
              EventManager::SendEvent("LR34");
            }
            if (Script::getInstance().GetFlag(25) == 1)
            {
              Script::getInstance().SetFlag(25, 0);
              EventManager::SendEvent("LR25Off");
              EventManager::SendEvent("LR35");
            }
            if (Script::getInstance().GetFlag(26) == 1)
            {
              Script::getInstance().SetFlag(26, 0);
              EventManager::SendEvent("LR26Off");
              EventManager::SendEvent("LR36");
            }
            if (Script::getInstance().GetFlag(27) == 1)
            {
              Script::getInstance().SetFlag(27, 0);
              EventManager::SendEvent("LR27Off");
              EventManager::SendEvent("LR37");
            }
            if (Script::getInstance().GetFlag(28) == 1)
            {
              Script::getInstance().SetFlag(28, 0);
              EventManager::SendEvent("LR28Off");
              EventManager::SendEvent("LR38");
            }
            if (Script::getInstance().GetFlag(29) == 1)
            {
              Script::getInstance().SetFlag(29, 0);
              EventManager::SendEvent("LR29Off");
              EventManager::SendEvent("LR39");
            }
          }
          else if (Script::getInstance().GetFlag(21) == 1)
          {
            Script::getInstance().SetFlag(20, 1);
            Script::getInstance().SetFlag(21, 0);
            EventManager::SendEvent("LR21Off");
            EventManager::SendEvent("LR31");
          }
        };
        /* Events for PlayScene */
        fnmap["PlayLoading"] = []() {
          Script::getInstance().SetFlag(80, 1);   // Loading
          Script::getInstance().SetFlag(81, 0);   // Loaded
          EventManager::SendEvent("LR40Off");     // READY
          EventManager::SendEvent("LR41Off");     // START
        };
        fnmap["PlayReady"] = []() {
          Script::getInstance().SetFlag(80, 0);
          Script::getInstance().SetFlag(81, 1);
          EventManager::SendEvent("LR40");        // READY
        };
        fnmap["PlayStart"] = []() {
          EventManager::SendEvent("LR41");        // START
        };
      }
      return fnmap;
    }

    class LR2EventReceiver : public EventReceiver
    {
      virtual bool OnEvent(const EventMessage &e)
      {
        auto &fnmap = GetEventFnMap();
        auto &i = fnmap.find(e.GetEventName());
        if (i == fnmap.end())
          return false;
        i->second();
        // Call redirected event instantly
        EventManager::Flush();
        return true;
      }
    };

    static LR2EventReceiver r;
    static bool is_event_subscribed = false;

    void HookEvent()
    {
      if (!is_event_subscribed)
      {
        is_event_subscribed = true;
        for (auto &i : GetEventFnMap())
          r.SubscribeTo(i.first);
      }
    }
    
    void UnHookEvent()
    {
      if (is_event_subscribed)
      {
        is_event_subscribed = false;
        r.UnsubscribeAll();
      }
    }
  }
}