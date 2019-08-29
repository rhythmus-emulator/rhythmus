#include "Event.h"
#include "SceneManager.h"
#include "Graphic.h" /* Need to get GLFW handle */
#include <mutex>
#include <GLFW/glfw3.h>

namespace rhythmus
{

/* Size of pre-reserved for event cache (to improve performance) */
constexpr int kPreCacheEventCount = 64;

// lock used when using cached_events_
std::mutex mtx_swap_lock;

// cached events.
// stored from input thread, flushed in rendering thread.
std::vector<EventMessage> evt_messages_;

// ------------------------- Internal Functions

void on_keyevent(GLFWwindow *w, int key, int scancode, int action, int mode)
{
  int eventid = 0;
  if (action == GLFW_PRESS)
    eventid = Events::kOnKeyDown;
  else if (action == GLFW_RELEASE)
    eventid = Events::kOnKeyUp;
  else if (action == GLFW_REPEAT)
    eventid = Events::kOnKeyPress;

  EventMessage msg(eventid, false);
  msg.SetParam(&key, 2);    // key & scancode
  EventManager::SendEvent(msg);
}

void on_text(GLFWwindow *w, uint32_t codepoint)
{
  EventManager::SendEvent(Events::kOnText);
}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
  EventManager::SendEvent(Events::kOnCursorMove);
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
  EventManager::SendEvent(Events::kOnCursorClick);
}

void on_joystick_conn(int jid, int event)
{
}


// ------------------------- class EventMessage

EventMessage::EventMessage(int id, bool time_synced)
  : id_(id)
{
  memset(params_, 0, sizeof(params_));
  if (time_synced) SetNewTime();
  else SetNewTimeUncached();
}

void EventMessage::SetDesc(const std::string& desc) { desc_ = desc; }
const std::string& EventMessage::GetDesc() const { return desc_; }
void EventMessage::SetEventID(int id) { id_ = id; }
int EventMessage::GetEventID() const { return id_; }

void EventMessage::SetNewTime()
{
  time_ = Timer::GetGameTime();
}

void EventMessage::SetNewTimeUncached()
{
  time_ = Timer::GetUncachedGameTime();
}

void EventMessage::SetParam(int *params, int param_len)
{
  while (param_len-- > 0)
    params_[param_len] = params[param_len];
}

bool EventMessage::IsKeyDown() const { return id_ == Events::kOnKeyDown; }
bool EventMessage::IsKeyPress() const { return id_ == Events::kOnKeyPress; /* indicates constant key press */ }
bool EventMessage::IsKeyUp() const { return id_ == Events::kOnKeyUp; }
int EventMessage::GetKeycode() const { return params_[0]; }


EventReceiver::~EventReceiver()
{
  UnsubscribeAll();
}


// ------------------------- class EventReceiver

void EventReceiver::SubscribeTo(int id)
{
  EventManager& em = EventManager::getInstance();
  em.event_subscribers_[id].insert(this);
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

EventManager::EventManager()
{
  // initialize events with name
  for (current_evtidx_ = 0; current_evtidx_ < Events::kEventLast; ++current_evtidx_)
  {
    if (!kEventNames[current_evtidx_])
      continue;
    evtname_to_evtid_[kEventNames[current_evtidx_]] = current_evtidx_;
  }
}

void EventManager::Subscribe(EventReceiver& e, int event_id)
{
  e.SubscribeTo(event_id);
}

void EventManager::Unsubscribe(EventReceiver& e)
{
  e.UnsubscribeAll();
}

bool EventManager::IsSubscribed(EventReceiver& e, int event_id)
{
  auto& evtlist = event_subscribers_[event_id];
  return evtlist.find(&e) != evtlist.end();
}

void EventManager::SendEvent(int event_id)
{
  EventMessage msg(event_id);
  SendEvent(msg);
}

void EventManager::SendEvent(const std::string& event_name)
{
  auto &e = getInstance();
  auto ii = e.evtname_to_evtid_.find(event_name);
  if (ii != e.evtname_to_evtid_.end())
    SendEvent(ii->second);
  else
  {
    e.evtname_to_evtid_[event_name] = ++e.current_evtidx_;
    SendEvent(e.current_evtidx_);
  }
}

void EventManager::SendEvent(const EventMessage &msg)
{
  mtx_swap_lock.lock();
  evt_messages_.push_back(msg);
  mtx_swap_lock.unlock();
}

void EventManager::SendEvent(EventMessage &&msg)
{
  mtx_swap_lock.lock();
  evt_messages_.emplace_back(msg);
  mtx_swap_lock.unlock();
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
  mtx_swap_lock.lock();
  evt_messages_.swap(evnt);
  mtx_swap_lock.unlock();

  for (const auto& e : evnt)
  {
    auto &evtlist = getInstance().event_subscribers_[e.GetEventID()];
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

}