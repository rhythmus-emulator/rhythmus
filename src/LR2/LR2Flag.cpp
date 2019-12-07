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
        fnmap["SceneLoaded"] = []() { EventManager::SendEvent("LR0"); };
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