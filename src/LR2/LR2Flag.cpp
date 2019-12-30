#include "LR2Flag.h"
#include "Event.h"
#include "Game.h"
#include "Song.h"
#include "Script.h"
#include "SceneManager.h"
#include "object/MusicWheel.h"
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
        fnmap["Load"] = []() {
          EventManager::SendEvent("LR0");
        };
        /* Events for SelectScene */
        fnmap["SelectSceneLoad"] = []() {
          /* Panel state clear */
          Script::getInstance().SetFlag(20, 1);
          Script::getInstance().SetFlag(21, 0);
          Script::getInstance().SetFlag(22, 0);
          Script::getInstance().SetFlag(23, 0);
          Script::getInstance().SetFlag(24, 0);
          Script::getInstance().SetFlag(25, 0);
          Script::getInstance().SetFlag(26, 0);
          Script::getInstance().SetFlag(27, 0);
          Script::getInstance().SetFlag(28, 0);
          Script::getInstance().SetFlag(29, 0);
          EventManager::SendEvent("Panel1Off");
          EventManager::SendEvent("Panel2Off");
          EventManager::SendEvent("Panel3Off");
          EventManager::SendEvent("Panel4Off");
          EventManager::SendEvent("Panel5Off");
          EventManager::SendEvent("Panel6Off");
          EventManager::SendEvent("Panel7Off");
          EventManager::SendEvent("Panel8Off");
          EventManager::SendEvent("Panel9Off");
          Script::getInstance().SetFlag(46, 0);
          Script::getInstance().SetFlag(47, 1);
        };
        fnmap["SongSelectChange"] = []() {
          EventManager::SendEvent("LR10");
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
          EventManager::SendEvent("LR10Off");
          EventManager::SendEvent("LR12Off");
          EventManager::SendEvent("LR13Off");
          EventManager::SendEvent("LR14");
        };
        /* Events for SelectScene Panel */
        static auto fnPanel = [](int panelidx) {
          const char* paneloffevents[] = { 0,
            "Panel1Off","Panel2Off","Panel3Off","Panel4Off","Panel5Off",
            "Panel6Off","Panel7Off","Panel8Off","Panel9Off",0
          };
          const char* panelonevents[] = { 0,
            "Panel1","Panel2","Panel3","Panel4","Panel5",
            "Panel6","Panel7","Panel8","Panel9",0
          };
          const char* panelonop2[] = { 0,
            "LR21","LR22","LR23","LR24","LR25",
            "LR26","LR27","LR28","LR29",0
          };
          const char* paneloffop2[] = { 0,
            "LR21Off","LR22Off","LR23Off","LR24Off","LR25Off",
            "LR26Off","LR27Off","LR28Off","LR29Off",0
          };
          const char* paneloffop3[] = { 0,
            "LR31","LR32","LR33","LR34","LR35",
            "LR36","LR37","LR38","LR39",0
          };
          const char* panelonop3[] = { 0,
            "LR31Off","LR32Off","LR33Off","LR34Off","LR35Off",
            "LR36Off","LR37Off","LR38Off","LR39Off",0
          };
          /* If my panel is on, then turn off my panel.
           * else, turn on my panel and turn off remaining panels. */
          if (Script::getInstance().GetFlag(20 + panelidx) == 1)
          {
            Script::getInstance().SetFlag(20, 1);
            Script::getInstance().SetFlag(20 + panelidx, 0);
            EventManager::SendEvent(paneloffop2[panelidx]);
            EventManager::SendEvent(paneloffop3[panelidx]);
            EventManager::SendEvent(paneloffevents[panelidx]);
            Script::getInstance().SetButtonNumber(panelidx, 0);
          }
          else
          {
            Script::getInstance().SetFlag(20, 0);
            Script::getInstance().SetFlag(20 + panelidx, 1);
            EventManager::SendEvent(panelonevents[panelidx]);
            EventManager::SendEvent(panelonop2[panelidx]);
            EventManager::SendEvent(panelonop3[panelidx]);
            Script::getInstance().SetButtonNumber(panelidx, 1);
            for (int i = 1; i < 10; ++i)
            {
              if (panelidx != i && Script::getInstance().GetFlag(20 + i) == 1)
              {
                Script::getInstance().SetFlag(20 + i, 0);
                EventManager::SendEvent(paneloffop2[i]);
                EventManager::SendEvent(paneloffop3[i]);
                EventManager::SendEvent(paneloffevents[i]);
                Script::getInstance().SetButtonNumber(i, 0);
              }
            }
          }
        };
        fnmap["Click1"] = []() { fnPanel(1); };
        fnmap["Click2"] = []() { fnPanel(2); };
        fnmap["Click3"] = []() { fnPanel(3); };
        fnmap["Click4"] = []() { fnPanel(4); };
        fnmap["Click5"] = []() { fnPanel(5); };
        fnmap["Click6"] = []() { fnPanel(6); };
        fnmap["Click7"] = []() { fnPanel(7); };
        fnmap["Click8"] = []() { fnPanel(8); };
        fnmap["Click9"] = []() { fnPanel(9); };
        fnmap["Click10"] = []() {
          // change difficulty filtering of MusicWheel
          // XXX: use dynamic_cast for safety?
          auto *wheel = static_cast<MusicWheel*>(
            SceneManager::get_current_scene()->FindChildByName("MusicWheel")
            );
          if (!wheel) return;
          static const int difficulty_filter[] = {
            Difficulty::kDifficultyNone,
            Difficulty::kDifficultyEasy,
            Difficulty::kDifficultyNormal,
            Difficulty::kDifficultyHard,
            Difficulty::kDifficultyEx,
            Difficulty::kDifficultyInsane,
          };
          int val = Script::getInstance().GetNumber(1010);
          val = (val + 1) % 6;
          Script::getInstance().SetFlag(46, val != 0);
          Script::getInstance().SetFlag(47, val == 0);
          wheel->SetDifficultyFilter(difficulty_filter[val]);
          wheel->RebuildData();
          Script::getInstance().SetButtonNumber(10, val);
          EventManager::SendEvent("LR11"); /* Song selection effect timer */
        };
        fnmap["Click11"] = []() {
          // change key filtering of MusicWheel
          auto *wheel = static_cast<MusicWheel*>(
            SceneManager::get_current_scene()->FindChildByName("MusicWheel")
            );
          if (!wheel) return;
          static const int key_filter[] = {
            Gamemode::kGamemodeNone,
            Gamemode::kGamemode5Key,    /* XXX: 5key but IIDX */
            Gamemode::kGamemodeIIDXSP,
            Gamemode::kGamemodeIIDXDP,  /* XXX: 10 key */
            Gamemode::kGamemodeIIDXDP,
            Gamemode::kGamemodePopn,
          };
          int val = Script::getInstance().GetNumber(1011);
          val = (val + 1) % 6;
          wheel->SetDifficultyFilter(key_filter[val]);
          wheel->RebuildData();
          Script::getInstance().SetButtonNumber(11, val);
          EventManager::SendEvent("LR11"); /* Song selection effect timer */
        };
        fnmap["Click12"] = []() {
          // change sort of MusicWheel
          auto *wheel = static_cast<MusicWheel*>(
            SceneManager::get_current_scene()->FindChildByName("MusicWheel")
            );
          if (!wheel) return;
          static const int sort_filter[] = {
            Sorttype::kNoSort,
            Sorttype::kSortByLevel,
            Sorttype::kSortByTitle,
            Sorttype::kSortByClear,
          };
          int val = Script::getInstance().GetNumber(1012);
          val = (val + 1) % 4;
          wheel->SetDifficultyFilter(sort_filter[val]);
          wheel->RebuildData();
          Script::getInstance().SetButtonNumber(12, val);
          Script::getInstance().SetButtonNumber(12, wheel->GetSort());
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