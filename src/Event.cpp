#include "Event.h"
#include "SceneManager.h"
#include "Graphic.h" /* Need to get GLFW handle */
#include <mutex>
#include <algorithm>
#include <GLFW/glfw3.h>

namespace rhythmus
{

// ------------------------ Input event related

/* Size of pre-reserved for event cache (to improve performance) */
constexpr int kPreCacheEventCount = 64;

// lock used when using cached_events_
std::mutex input_evt_lock;

// cached events.
// stored from input thread, flushed in rendering thread.
std::vector<InputEvent> input_evt_messages_;
std::vector<InputEventReceiver*> input_evt_receivers_;

std::mutex game_evt_lock;
std::vector<EventMessage> game_evt_messages_;

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

InputEvent::InputEvent(int type) : type_(type)
{
  memset(argv_, 0, sizeof(argv_));
  time_ = Timer::GetUncachedGameTime();
}

int InputEvent::type() const { return type_; }
double InputEvent::time() const { return time_; }

void InputEvent::SetKeyCode(int v) { argv_[0] = v; }
void InputEvent::SetPosition(int x, int y) { argv_[0] = x; argv_[1] = y; }
void InputEvent::SetCodepoint(uint32_t codepoint) { *(uint32_t*)argv_ = codepoint; }

int InputEvent::KeyCode() const { return argv_[0]; }
int InputEvent::GetX() const { return argv_[0]; }
int InputEvent::GetY() const { return argv_[1]; }
uint32_t InputEvent::Codepoint() const { return *(uint32_t*)argv_; }

void on_keyevent(GLFWwindow *w, int key, int scancode, int action, int mode)
{
  int eventid = 0;
  if (action == GLFW_PRESS)
    eventid = InputEvents::kOnKeyDown;
  else if (action == GLFW_RELEASE)
    eventid = InputEvents::kOnKeyUp;
  else if (action == GLFW_REPEAT)
    eventid = InputEvents::kOnKeyPress;

  InputEvent msg(eventid);
  msg.SetKeyCode(scancode);

  {
    std::lock_guard<std::mutex> lock(input_evt_lock);
    input_evt_messages_.push_back(msg);
  }
}

void on_text(GLFWwindow *w, uint32_t codepoint)
{
  InputEvent msg(InputEvents::kOnText);
  msg.SetCodepoint(codepoint);

  {
    std::lock_guard<std::mutex> lock(input_evt_lock);
    input_evt_messages_.push_back(msg);
  }
}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
  InputEvent msg(InputEvents::kOnCursorMove);
  msg.SetPosition(xpos, ypos);

  {
    std::lock_guard<std::mutex> lock(input_evt_lock);
    input_evt_messages_.push_back(msg);
  }
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
  InputEvent msg(InputEvents::kOnCursorClick);
  //msg.SetCodepoint(codepoint);

  {
    std::lock_guard<std::mutex> lock(input_evt_lock);
    input_evt_messages_.push_back(msg);
  }
}

void on_joystick_conn(int jid, int event)
{
}

void InputEventManager::Flush()
{
  std::vector<InputEvent> events(kPreCacheEventCount);
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

EventMessage::EventMessage(const std::string &name)
  : name_(name)
{
}

void EventMessage::SetEventName(const std::string &name) { name_ = name; }
const std::string &EventMessage::GetEventName() const { return name_; }


// ------------------------- class EventReceiver

EventReceiver::~EventReceiver()
{
  UnsubscribeAll();
}

void EventReceiver::SubscribeTo(const std::string &name)
{
  EventManager& em = EventManager::getInstance();
  subscription_.push_back(name);
  em.event_subscribers_[name].insert(this);
}

void EventReceiver::UnsubscribeAll()
{
  EventManager& em = EventManager::getInstance();
  for (auto eid : subscription_)
  {
    auto& sublist = em.event_subscribers_[eid];
    if (sublist.find(this) != sublist.end())
      sublist.erase(this);
  }
}

bool EventReceiver::OnEvent(const EventMessage& msg)
{
  // return false will stop event propagation.
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

/* @brief Subscribe to input events */
void EventManager::Initialize()
{
  auto* window_ = Graphic::getInstance().window();
  ASSERT(window_);
  glfwSetKeyCallback(window_, on_keyevent);
  glfwSetCharCallback(window_, on_text);
  glfwSetCursorPosCallback(window_, on_cursormove);
  glfwSetMouseButtonCallback(window_, on_cursorbutton);
  glfwSetJoystickCallback(on_joystick_conn);
}

void EventManager::Flush()
{
  auto& em = getInstance();

  // process cached events.
  // swap cache to prevent event lagging while ProcessEvent.
  // Pre-reserve enough space for new event cache to improve performance.
  std::vector<EventMessage> evnt(kPreCacheEventCount);
  {
    std::lock_guard<std::mutex> lock(game_evt_lock);
    game_evt_messages_.swap(evnt);
  }

  for (const auto& e : evnt)
  {
    auto &evtlist = getInstance().event_subscribers_[e.GetEventName()];
    for (auto* recv : evtlist)
    {
      if (!recv->OnEvent(e))
        break;
    }
    // Depreciated: Send remain event to SceneManager.
    //SceneManager::getInstance().SendEvent(e);
  }
}

EventManager& EventManager::getInstance()
{
  static EventManager em;
  return em;
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
      EventManager::SendEvent(ii->name);
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
    EventManager::SendEvent(e.name);
  }
  events_.clear();
}

void QueuedEventCache::DeleteAll()
{
  events_.clear();
}


}