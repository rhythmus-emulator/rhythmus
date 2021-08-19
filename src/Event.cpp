#include "Event.h"
#include "SceneManager.h"
#include "Logger.h"
#include "Graphic.h" /* Need to get GLFW handle */
#include "common.h"
#include "config.h"

#if USE_LR2_FEATURE == 1
#include "KeyPool.h"
#endif

#include <mutex>
#include <GLFW/glfw3.h>

namespace rhythmus
{

// ------------------------ Input event related

/* Size of pre-reserved for event cache (to improve performance) */
constexpr int kPreCacheEventCount = 64;

// lock used when using cached_events_
std::mutex input_evt_lock;

// lock used when subscribe/unsubscribe event
std::mutex gSubscribeLock;

// cached events.
// stored from input thread, flushed in rendering thread.
std::vector<InputEvent> input_evt_messages_;
std::vector<InputEventReceiver*> input_evt_receivers_;

std::mutex game_evt_lock;
std::vector<EventMessage> game_evt_messages_;
int window_x, window_y;
double cursor_x, cursor_y;

InputEventReceiver::InputEventReceiver()
{
  std::lock_guard<std::mutex> lock(input_evt_lock);
  input_evt_receivers_.push_back(this);
}

InputEventReceiver::~InputEventReceiver()
{
  std::lock_guard<std::mutex> lock(input_evt_lock);
  auto it = std::find(input_evt_receivers_.begin(), input_evt_receivers_.end(), this);
  if (it != input_evt_receivers_.end())
    input_evt_receivers_.erase(it);
}

InputEvent::InputEvent() : type_(0), time_(0)
{
  // XXX: should not be called ...?
  memset(argv_, 0, sizeof(argv_));
}

InputEvent::InputEvent(int type) : type_(type)
{
  memset(argv_, 0, sizeof(argv_));
  time_ = Timer::GetUncachedSystemTime();
}

int InputEvent::type() const { return type_; }
double InputEvent::time() const { return time_; }

void InputEvent::SetKeyCode(int v) { argv_[0] = v; }
void InputEvent::SetPosition(int x, int y) { argv_[0] = x; argv_[1] = y; }
void InputEvent::SetButton(int button) { argv_[2] = button; }
void InputEvent::SetCodepoint(uint32_t codepoint) { *(uint32_t*)argv_ = codepoint; }

int InputEvent::KeyCode() const { return argv_[0]; }
int InputEvent::GetX() const { return argv_[0]; }
int InputEvent::GetY() const { return argv_[1]; }
int InputEvent::GetButton() const { return argv_[2]; }
uint32_t InputEvent::Codepoint() const { return *(uint32_t*)argv_; }

std::string s_cmd;
bool s_cmd_mode = false;

void on_keyevent(GLFWwindow *w, int key, int scancode, int action, int mode)
{
  int eventid = 0;
  if (action == GLFW_PRESS)
    eventid = InputEvents::kOnKeyDown;
  else if (action == GLFW_RELEASE)
    eventid = InputEvents::kOnKeyUp;
  else if (action == GLFW_REPEAT)
    eventid = InputEvents::kOnKeyPress;

  /* process for special command */
  if (action == GLFW_RELEASE) {
    if (!s_cmd_mode && key == GLFW_KEY_TAB && (mode & GLFW_MOD_CONTROL) && (mode & GLFW_MOD_SHIFT)) {
      s_cmd_mode = true;
      GAME->SetTitle("Command Mode Entered");
      return;
    }
    else if (s_cmd_mode) {
      if (key == GLFW_KEY_V && (mode & GLFW_MOD_CONTROL)) {
        s_cmd = glfwGetClipboardString(w);
        GAME->RunCommand(s_cmd);
        s_cmd.clear();
        s_cmd_mode = false;
      }
      else if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) {
        if (!s_cmd.empty()) s_cmd.pop_back();
      }
      else if (key == GLFW_KEY_ESCAPE) {
        s_cmd.clear();
        s_cmd_mode = false;
      }
      else if (key == GLFW_KEY_ENTER) {
        GAME->RunCommand(s_cmd);
        s_cmd.clear();
        s_cmd_mode = false;
      }
      else if (key <= 0xFF) {
        s_cmd.push_back((char)key);
      }

      if (s_cmd_mode) {
        GAME->SetTitle("Command: " + s_cmd);
      }
      else {
        GAME->ResetTitle();
      }

      return;
    }
  }

#if 0
  /* TODO: if paste, then override text to OnText */
  if (key == GLFW_KEY_V && (mode & GLFW_MOD_CONTROL)) {
  }
#endif

  InputEvent msg(eventid);
  msg.SetKeyCode(key);
  input_evt_messages_.push_back(msg);
}

void on_text(GLFWwindow *w, uint32_t codepoint)
{
  InputEvent msg(InputEvents::kOnText);
  msg.SetCodepoint(codepoint);
  input_evt_messages_.push_back(msg);
}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
  InputEvent msg(InputEvents::kOnCursorMove);
  cursor_x = xpos;
  cursor_y = ypos;
  if (window_x && window_y)
  {
    cursor_x = cursor_x / GRAPHIC->width() * window_x;
    cursor_y = cursor_y / GRAPHIC->height() * window_y;
  }
  msg.SetPosition((int)(cursor_x + 0.5), (int)(cursor_y + 0.5));
  input_evt_messages_.push_back(msg);
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
  InputEvent msg(
    action == GLFW_PRESS ? InputEvents::kOnCursorDown : InputEvents::kOnCursorClick
  );
  msg.SetButton(button);
  msg.SetPosition((int)(cursor_x + 0.5), (int)(cursor_y + 0.5));
  input_evt_messages_.push_back(msg);
}

void on_joystick_conn(int jid, int event)
{
  // XXX: need lock_guard as joystick event is asynchronous
}

void on_window_resize(GLFWwindow *w, int width, int height)
{
  window_x = width;
  window_y = height;
}

void InputEventManager::Flush()
{
  std::vector<InputEvent> events;
  events.reserve(kPreCacheEventCount);
  {
    std::lock_guard<std::mutex> lock(input_evt_lock);
    events.swap(input_evt_messages_);
  }

  for (auto &e : events)
  {
    for (auto *subscriber : input_evt_receivers_)
      subscriber->OnInputEvent(e);
  }
}


// ------------------------- class EventMessage

EventMessage::EventMessage(const std::string& name)
  : name_(name)
{
}

EventMessage::EventMessage(const std::string& name, const std::string& content)
  : name_(name), content_(content)
{
}

void EventMessage::SetEventName(const std::string &name) { name_ = name; }
void EventMessage::SetContent(const std::string& content) { content_ = content; }
const std::string& EventMessage::GetEventName() const { return name_; }
const std::string& EventMessage::content() const { return content_; }


// ------------------------- class EventReceiver

EventReceiver::~EventReceiver()
{
  UnsubscribeAll();
}

void EventReceiver::SubscribeTo(const std::string &name)
{
  std::lock_guard<std::mutex> l(gSubscribeLock);
  subscription_.push_back(name);
  EVENTMAN->event_subscribers_[name].insert(this);
}

void EventReceiver::UnsubscribeAll()
{
  std::lock_guard<std::mutex> l(gSubscribeLock);
  for (auto eid : subscription_)
  {
    auto& sublist = EVENTMAN->event_subscribers_[eid];
    if (sublist.find(this) != sublist.end())
      sublist.erase(this);
  }
}

bool EventReceiver::OnEvent(const EventMessage& msg)
{
  // return false will stop event propagation.
  return true;
}

// ------------------------- class EventReceiverMap

EventReceiverMap::~EventReceiverMap() {}

void EventReceiverMap::AddEvent(const std::string &event_name, const std::function<void (const EventMessage&)> &func)
{
  auto i = eventFnMap_.find(event_name);
  eventFnMap_[event_name] = func;
  if (i == eventFnMap_.end()) {
    SubscribeTo(event_name);
  }
}

void EventReceiverMap::Clear()
{
  UnsubscribeAll();
  eventFnMap_.clear();
}

bool EventReceiverMap::OnEvent(const EventMessage& msg)
{
  auto i = eventFnMap_.find(msg.GetEventName());
  R_ASSERT(i != eventFnMap_.end());
  i->second(msg);
  return true;
}

// ------------------------- class EventManager

#if 0
/* system-defined event names in string */
const char* kEventNames[Events::kEventLast] = {
  "None",

  "KeyDown",
  "KeyPress",
  "KeyUp",
  "KeyText",
  "CursorMove",
  "CursorClick",
  "Joystick",

  "SceneChanged",
  "SongSelectChanged",
  "SongSelected",
  "PlayStarted",
  "PlayAborted",
  "Cleared",

  0,
};
#endif

EventManager::EventManager()
{
}

EventManager::~EventManager()
{
  // clear all subscriber.
  // use game event lock to prevent call Flush() method while cleanup.
  std::lock_guard<std::mutex> lock(game_evt_lock);
  for (auto i : event_subscribers_)
  {
    for (auto *e : i.second)
      e->subscription_.clear();
  }
  event_subscribers_.clear();
}

void EventManager::Subscribe(EventReceiver& e, const std::string &name)
{
  e.SubscribeTo(name);
}

void EventManager::Unsubscribe(EventReceiver& e)
{
  e.UnsubscribeAll();
}

bool EventManager::IsSubscribed(EventReceiver& e, const std::string &name)
{
  auto& evtlist = event_subscribers_[name];
  return evtlist.find(&e) != evtlist.end();
}

void EventManager::SendEvent(const std::string& event_name)
{
  SendEvent(EventMessage(event_name));
}

void EventManager::SendEvent(const EventMessage &msg)
{
  std::lock_guard<std::mutex> lock(game_evt_lock);
  game_evt_messages_.push_back(msg);
}

void EventManager::SendEvent(EventMessage &&msg)
{
  std::lock_guard<std::mutex> lock(game_evt_lock);
  game_evt_messages_.emplace_back(msg);
}

#if USE_LR2_FEATURE == 1
/* @brief Manages KeyValue set for LR2 and propagates values. */
class LR2KeyPool
{
public:
  LR2KeyPool() :
    info_title(KEYPOOL->GetString("MusicWheelTitle")),
    info_subtitle(KEYPOOL->GetString("MusicWheelSubTitle")),
    info_fulltitle(KEYPOOL->GetString("MusicWheelFullTitle")),
    info_genre(KEYPOOL->GetString("MusicWheelGenre")),
    info_artist(KEYPOOL->GetString("MusicWheelArtist")),
    info_itemtype(KEYPOOL->GetInt("MusicWheelType")),
    info_diff(KEYPOOL->GetInt("MusicWheelDiff")),
    info_bpmmax(KEYPOOL->GetInt("MusicWheelBpmMax")),
    info_bpmmin(KEYPOOL->GetInt("MusicWheelBpmMin")),
    info_level(KEYPOOL->GetInt("MusicWheelLevel")),
    info_difftype_1(KEYPOOL->GetInt("MusicWheelDT1")),
    info_difftype_2(KEYPOOL->GetInt("MusicWheelDT2")),
    info_difftype_3(KEYPOOL->GetInt("MusicWheelDT3")),
    info_difftype_4(KEYPOOL->GetInt("MusicWheelDT4")),
    info_difftype_5(KEYPOOL->GetInt("MusicWheelDT5")),
    info_difflv_1(KEYPOOL->GetInt("MusicWheelDLV1")),
    info_difflv_2(KEYPOOL->GetInt("MusicWheelDLV2")),
    info_difflv_3(KEYPOOL->GetInt("MusicWheelDLV3")),
    info_difflv_4(KEYPOOL->GetInt("MusicWheelDLV4")),
    info_difflv_5(KEYPOOL->GetInt("MusicWheelDLV5")),
    info_score(KEYPOOL->GetInt("MusicWheelScore")),
    info_exscore(KEYPOOL->GetInt("MusicWheelExScore")),
    info_totalnote(KEYPOOL->GetInt("MusicWheelTotalNote")),
    info_maxcombo(KEYPOOL->GetInt("MusicWheelMaxCombo")),
    info_playcount(KEYPOOL->GetInt("MusicWheelPlayCount")),
    info_clearcount(KEYPOOL->GetInt("MusicWheelClearCount")),
    info_failcount(KEYPOOL->GetInt("MusicWheelFailCount")),
    info_cleartype(KEYPOOL->GetInt("MusicWheelClearType")),
    info_pg(KEYPOOL->GetInt("MusicWheelPG")),
    info_gr(KEYPOOL->GetInt("MusicWheelGR")),
    info_gd(KEYPOOL->GetInt("MusicWheelGD")),
    info_bd(KEYPOOL->GetInt("MusicWheelBD")),
    info_pr(KEYPOOL->GetInt("MusicWheelPR")),
    info_musicwheelpos(KEYPOOL->GetFloat("MusicWheelPos")) {}

  void Initialize()
  {
    char l[10];
    for (unsigned i = 0; i < 1000; ++i)
    {
      sprintf(l, "F%u", i);
      F[i] = KEYPOOL->GetInt(l);
    }
    for (unsigned i = 0; i < 1000; ++i)
    {
      sprintf(l, "N%u", i);
      N[i] = KEYPOOL->GetInt(l);
    }
    for (unsigned i = 0; i < 1000; ++i)
    {
      sprintf(l, "S%u", i);
      S[i] = KEYPOOL->GetString(l);
    }
    for (unsigned i = 0; i < 1000; ++i)
    {
      sprintf(l, "button%u", i);
      button[i] = KEYPOOL->GetInt(l);
    }
    for (unsigned i = 0; i < 100; ++i)
    {
      sprintf(l, "bargraph%u", i);
      bargraph[i] = KEYPOOL->GetFloat(l);
    }
    for (unsigned i = 0; i < 100; ++i)
    {
      sprintf(l, "slider%u", i);
      slider[i] = KEYPOOL->GetFloat(l);
    }


    /* flag initialization */
    *F[0] = 1;
    // XXX: by default ...? or in select scene start?
    *F[50] = 1;   // OFFLINE
    *F[52] = 1;   // EXTRA MODE OFF


    /* create EventMap */
    fnmap.AddEvent("Load", [this](const EventMessage&) {
      EVENTMAN->SendEvent("LR0");
    });

    /* Events for SelectScene */
    fnmap.AddEvent("SelectSceneLoad", [this](const EventMessage&) {
      /* Panel state clear */
      *F[20] = 1;
      *F[21] = 0;
      *F[22] = 0;
      *F[23] = 0;
      *F[24] = 0;
      *F[25] = 0;
      *F[26] = 0;
      *F[27] = 0;
      *F[28] = 0;
      *F[29] = 0;
      EVENTMAN->SendEvent("LR21Off");
      EVENTMAN->SendEvent("LR22Off");
      EVENTMAN->SendEvent("LR23Off");
      EVENTMAN->SendEvent("LR24Off");
      EVENTMAN->SendEvent("LR25Off");
      EVENTMAN->SendEvent("LR26Off");
      EVENTMAN->SendEvent("LR27Off");
      EVENTMAN->SendEvent("LR28Off");
      EVENTMAN->SendEvent("LR29Off");
      *F[46] = 0;
      *F[47] = 1;
    });

    fnmap.AddEvent("SongFilterChanged", [this](const EventMessage&) {
      /* update filter information */
      int gamemode = KEYPOOL->GetInt("gamemode").get();
      int sortmode = KEYPOOL->GetInt("sortmode").get();
      int difficulty = KEYPOOL->GetInt("difficulty").get();

      /* ALL, BEGINNER, NORMAL, HYPER, HARD, ANOTHER, INSANE */
      static const int difficulty_filter[] = { 0, 1, 2, 3, 4, 5, 6 };
      /* ALL,5Key,SP,10Key,DP,9Key */
      static const int key_filter[] = { 0, 1, 3, 6, 7, 5 };
      /* NOSORT(NAME), LEVEL, NAME, CLEAR */
      static const int sort_filter[4] = { 0, 1, 2, 4 };

      for (int i = 0; i < 7; ++i) if (difficulty <= difficulty_filter[i]) {
        *F[46] = (i != 0);
        *F[47] = (i == 0);
        *button[10] = i;
        break;
      }
      for (int i = 0; i < 6; ++i) if (gamemode <= key_filter[i]) {
        *button[11] = i;
        break;
      }
      for (int i = 0; i < 4; ++i) if (sortmode == sort_filter[i]) {
        *button[12] = i;
        break;
      }
    });

    fnmap.AddEvent("SongSelectChanged", [this](const EventMessage&) {
      int diff_not_exist[5] = {
        *info_difftype_1 < 0,
        *info_difftype_2 < 0,
        *info_difftype_3 < 0,
        *info_difftype_4 < 0,
        *info_difftype_5 < 0,
      };

      int diff_exist[5] = {
        *info_difftype_1 >= 0,
        *info_difftype_2 >= 0,
        *info_difftype_3 >= 0,
        *info_difftype_4 >= 0,
        *info_difftype_5 >= 0,
      };

      int levels[5] = {
        *info_difflv_1,
        *info_difflv_2,
        *info_difflv_3,
        *info_difflv_4,
        *info_difflv_5,
      };

      /* Flag for difficulty of current song. */
      *S[10] = *info_title;
      *S[11] = *info_subtitle;
      *S[12] = *info_fulltitle;
      *S[13] = *info_genre;
      *S[14] = *info_artist;
      /* BPM */
      *N[90] = *info_bpmmax;
      *N[91] = *info_bpmmin;
      *F[176] = *info_bpmmax == *info_bpmmin;
      *F[177] = *info_bpmmax != *info_bpmmin;
      /* IR */
      *N[92] = 0;
      *N[93] = 0;
      *N[94] = 0;

      for (size_t i = 0; i < 5; ++i)
      {
        *F[500 + i] = diff_not_exist[i];
        *F[505 + i] = diff_exist[i];
        *F[510 + i] = *info_diff > 0;   // TODO: single
        *F[515 + i] = 0;                // TODO: multiple
        *N[45 + i] = *info_level;
        *F[151 + i] = (*info_diff - 1 == i);
      }
      *F[150] = *info_diff == 0;

      //for (size_t i = 1; i < 6; ++i)
      //  *F[i] = (*info_itemtype == i);  // Flag code?

      float rate = (float)(*info_exscore) / *info_totalnote / 2 * 100;
      int totalnote = *info_totalnote;
      *N[70] = *info_score;
      *N[71] = *info_exscore;
      *N[72] = totalnote * 2;
      *N[73] = (int)rate;
      *N[74] = totalnote;
      *N[75] = *info_maxcombo;
      *N[76] = *info_bd + *info_pr;
      *N[77] = *info_playcount;
      *N[78] = *info_clearcount;
      *N[79] = *info_failcount;
      *N[80] = *info_pg;
      *N[81] = *info_gr;
      *N[82] = *info_gd;
      *N[83] = *info_bd;
      *N[84] = *info_pr;
      *N[85] = static_cast<int>(*info_pg * 100.0f / totalnote);
      *N[85] = static_cast<int>(*info_gr * 100.0f / totalnote);
      *N[85] = static_cast<int>(*info_gd * 100.0f / totalnote);
      *N[85] = static_cast<int>(*info_bd * 100.0f / totalnote);
      *N[85] = static_cast<int>(*info_pr * 100.0f / totalnote);

      for (int i = 0; i <= 45; ++i)
        *F[520 + i] = 0;

      if (*info_itemtype == 2 /* if song */)
      {
        int clridx = *info_cleartype;
        int diff = *info_diff;
        *F[520 + diff * 10 + clridx] = 1;
      }

      *slider[1] = *info_musicwheelpos;

      EVENTMAN->SendEvent("LR10");
      EVENTMAN->SendEvent("LR11");
      EVENTMAN->SendEvent("LR14Off");
    });

    fnmap.AddEvent("SongSelectChangeUp", [](const EventMessage&) {
      EVENTMAN->SendEvent("LR12");
    });

    fnmap.AddEvent("SongSelectChangeDown", [](const EventMessage&) {
      EVENTMAN->SendEvent("LR13");
    });

    fnmap.AddEvent("SongSelectChanged", [](const EventMessage&) {
      EVENTMAN->SendEvent("LR10Off");
      EVENTMAN->SendEvent("LR12Off");
      EVENTMAN->SendEvent("LR13Off");
      EVENTMAN->SendEvent("LR14");
    });

    /* Events for SelectScene Panel */
    static auto fnPanel = [this](int panelidx) {
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
      if (*F[20 + panelidx] == 1)
      {
        *F[20] = 1;
        *F[20 + panelidx] = 0;
        EVENTMAN->SendEvent(paneloffop2[panelidx]);
        EVENTMAN->SendEvent(paneloffop3[panelidx]);
        EVENTMAN->SendEvent(paneloffevents[panelidx]);
        *button[panelidx] = 0;
      }
      else
      {
        *F[20] = 0;
        *F[20 + panelidx] = 1;
        EVENTMAN->SendEvent(panelonevents[panelidx]);
        EVENTMAN->SendEvent(panelonop2[panelidx]);
        EVENTMAN->SendEvent(panelonop3[panelidx]);
        *button[panelidx] = 1;
        for (int i = 1; i < 10; ++i)
        {
          if (panelidx != i && *F[20 + i] == 1)
          {
            *F[20 + i] = 0;
            EVENTMAN->SendEvent(paneloffop2[i]);
            EVENTMAN->SendEvent(paneloffop3[i]);
            EVENTMAN->SendEvent(paneloffevents[i]);
            *button[i] = 0;
          }
        }
      }
    };

    fnmap.AddEvent("Click1", [](const EventMessage&) { fnPanel(1); });
    fnmap.AddEvent("Click2", [](const EventMessage&) { fnPanel(2); });
    fnmap.AddEvent("Click3", [](const EventMessage&) { fnPanel(3); });
    fnmap.AddEvent("Click4", [](const EventMessage&) { fnPanel(4); });
    fnmap.AddEvent("Click5", [](const EventMessage&) { fnPanel(5); });
    fnmap.AddEvent("Click6", [](const EventMessage&) { fnPanel(6); });
    fnmap.AddEvent("Click7", [](const EventMessage&) { fnPanel(7); });
    fnmap.AddEvent("Click8", [](const EventMessage&) { fnPanel(8); });
    fnmap.AddEvent("Click9", [](const EventMessage&) { fnPanel(9); });
    fnmap.AddEvent("Click10", [](const EventMessage&) {
#if 0
      // change difficulty filtering of MusicWheel
      // XXX: use dynamic_cast for safety?
      // TODO: make and send message to SelectScene, and process from it.
      auto *wheel = static_cast<MusicWheel*>(
        SceneManager::get_current_scene()->FindChildByName("MusicWheel")
        );
      if (!wheel) return;
      wheel->NextDifficultyFilter();
      wheel->RebuildData();
#endif
      EVENTMAN->SendEvent("SongSelectChange");
    });
    fnmap.AddEvent("Click11", [](const EventMessage&) {
#if 0
      // change key filtering of MusicWheel
      // TODO: make and send message to SelectScene, and process from it.
      auto *wheel = static_cast<MusicWheel*>(
        SceneManager::get_current_scene()->FindChildByName("MusicWheel")
        );
      if (!wheel) return;
      wheel->NextGamemodeFilter();
      wheel->RebuildData();
#endif
      EVENTMAN->SendEvent("SongSelectChange");
    });
    fnmap.AddEvent("Click12", [](const EventMessage&) {
#if 0
      // change sort of MusicWheel
      // TODO: make and send message to SelectScene, and process from it.
      auto *wheel = static_cast<MusicWheel*>(
        SceneManager::get_current_scene()->FindChildByName("MusicWheel")
        );
      if (!wheel) return;
      wheel->NextSort();
      wheel->RebuildData();
#endif
      EVENTMAN->SendEvent("SongSelectChange");
    });
    /* Events for PlayScene */
    fnmap.AddEvent("PlayLoading", [this](const EventMessage&) {
      *F[80] = 1;   // Loading
      *F[81] = 1;   // Loaded
      EVENTMAN->SendEvent("LR40Off");     // READY
      EVENTMAN->SendEvent("LR41Off");     // START
    });
    fnmap.AddEvent("PlayReady", [this](const EventMessage&) {
      *F[80] = 0;   // Loading
      *F[81] = 1;   // Loaded
      EVENTMAN->SendEvent("LR40");        // READY
    });
    fnmap.AddEvent("PlayStart", [this](const EventMessage&) {
      EVENTMAN->SendEvent("LR41");        // START
    });
  }

  void Update()
  {
    /* SelectScene */


    /* ReadyScene */


    /* PlayScene */
  }

  // LR2 flags
  KeyData<int> F[1000];

  // LR2 numbers
  KeyData<int> N[1000];

  // LR2 string
  KeyData<std::string> S[1000];

  // LR2 buttons
  KeyData<int> button[1000];

  // LR2 bargraph
  KeyData<float> bargraph[100];

  // LR2 slider
  KeyData<float> slider[100];

  // Game used KeyPools
  KeyData<std::string> info_title;
  KeyData<std::string> info_subtitle;
  KeyData<std::string> info_fulltitle;
  KeyData<std::string> info_genre;
  KeyData<std::string> info_artist;
  KeyData<int> info_itemtype;
  KeyData<int> info_diff;
  KeyData<int> info_bpmmax;
  KeyData<int> info_bpmmin;
  KeyData<int> info_level;
  KeyData<int> info_difftype_1;
  KeyData<int> info_difftype_2;
  KeyData<int> info_difftype_3;
  KeyData<int> info_difftype_4;
  KeyData<int> info_difftype_5;
  KeyData<int> info_difflv_1;
  KeyData<int> info_difflv_2;
  KeyData<int> info_difflv_3;
  KeyData<int> info_difflv_4;
  KeyData<int> info_difflv_5;
  KeyData<int> info_score;
  KeyData<int> info_exscore;
  KeyData<int> info_totalnote;
  KeyData<int> info_maxcombo;
  KeyData<int> info_playcount;
  KeyData<int> info_clearcount;
  KeyData<int> info_failcount;
  KeyData<int> info_cleartype;
  KeyData<int> info_pg;
  KeyData<int> info_gr;
  KeyData<int> info_gd;
  KeyData<int> info_bd;
  KeyData<int> info_pr;
  KeyData<float> info_musicwheelpos;

  // EventMap
  EventReceiverMap fnmap;
};
#endif

/* @brief Subscribe to input events */
void EventManager::Initialize()
{
  EVENTMAN = new EventManager();

#if USE_GLFW
  GLFWwindow* window_ = (GLFWwindow*)GAME->handler();
  R_ASSERT(window_);
  glfwSetKeyCallback(window_, on_keyevent);
  glfwSetCharCallback(window_, on_text);
  glfwSetCursorPosCallback(window_, on_cursormove);
  glfwSetMouseButtonCallback(window_, on_cursorbutton);
  glfwSetJoystickCallback(on_joystick_conn);
  glfwSetWindowSizeCallback(window_, on_window_resize);
#endif

#if USE_LR2_FEATURE
  // enable LR2 event hooking
  static LR2KeyPool lr2keypool;
  lr2keypool.Initialize();
#endif
}

void EventManager::Cleanup()
{
  delete EVENTMAN;
}

int EventManager::GetStatus(unsigned input_id) const
{
  if (input_id >= RI_MOUSE_BUTTON_1)
    return 0; /* not implemented */
#if USE_GLFW
  if (glfwGetKey((GLFWwindow*)GAME->handler(), input_id) == GLFW_PRESS)
    return 1;
#endif
  return 0;
}

void EventManager::Flush()
{
  // process cached events until no remaining event to process.
  // swap cache to prevent event lagging while ProcessEvent.
  // Pre-reserve enough space for new event cache to improve performance.
  // @warn
  // In some case, infinite loop of event may caused.
  // By setting maximum event depth, infinite event loop is prevented.
  constexpr size_t kMaxEventDepth = 50;
  size_t event_depth = 0;
  std::vector<EventMessage> evnt(kPreCacheEventCount);
  while (!game_evt_messages_.empty())
  {
    if (event_depth > kMaxEventDepth)
    {
      Logger::Error("Event depth is too deep (over %d)", kMaxEventDepth);
      break;
    }
    evnt.clear();
    {
      std::lock_guard<std::mutex> lock(game_evt_lock);
      game_evt_messages_.swap(evnt);
    }

    for (const auto& e : evnt)
    {
      auto &evtlist = EVENTMAN->event_subscribers_[e.GetEventName()];
      for (auto* recv : evtlist)
      {
        if (!recv->OnEvent(e))
          break;
      }
      // Depreciated: Send remain event to SceneManager.
      //SceneManager::getInstance().SendEvent(e);
    }
    ++event_depth;
  }
}


// ------------------------------- QueuedEvents

QueuedEventCache::QueuedEventCache() : allow_queue_(true) {}

void QueuedEventCache::QueueEvent(const std::string &name, float timeout_msec)
{
  events_.emplace_back(QueuedEvent{ name, timeout_msec });
}

float QueuedEventCache::GetRemainingTime(const std::string& queue_name)
{
  if (queue_name.empty()) return 0;
  for (auto &e : events_)
  {
    if (e.name == queue_name)
      return e.time_delta;
  }
  return 0;
}

bool QueuedEventCache::IsQueued(const std::string& queue_name)
{
  if (queue_name.empty()) return false;
  for (auto &e : events_)
  {
    if (e.name == queue_name)
      return true;
  }
  return false;
}

void QueuedEventCache::Update(float delta)
{
  for (auto ii = events_.begin(); ii != events_.end(); )
  {
    ii->time_delta -= delta;
    if (ii->time_delta < 0)
    {
      EVENTMAN->SendEvent(ii->name);
      auto ii_e = ii;
      ii++;
      events_.erase(ii_e);
    }
    else ii++;
  }
}

void QueuedEventCache::SetQueueable(bool allow_queue)
{
  allow_queue_ = allow_queue;
}

void QueuedEventCache::FlushAll()
{
  for (auto &e : events_)
  {
    EVENTMAN->SendEvent(e.name);
  }
  events_.clear();
}

void QueuedEventCache::DeleteAll()
{
  events_.clear();
}

EventManager *EVENTMAN = nullptr;

}
