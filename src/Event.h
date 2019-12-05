#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>
#include <list>

namespace rhythmus
{

/**
 * @brief
 * General game event types (including input event)
 */
enum InputEvents
{
  kOnKeyDown,
  kOnKeyPress,  /* key event repeating due to pressing */
  kOnKeyUp,
  kOnText,
  kOnCursorMove,
  kOnCursorClick,
  kOnJoystick,

  kInputEventLast    /* unused event; just for last index */
};

class InputEventManager
{
public:
  static void Flush();
};

/* @brief Only for Input related event */
class InputEvent
{
public:
  InputEvent();
  InputEvent(int type);

  /* type index of enum InputEvents */
  int type() const;

  /* time in second. unsynced from gametime (means exact time) */
  double time() const;

  void SetKeyCode(int v);
  void SetPosition(int x, int y);
  void SetCodepoint(uint32_t codepoint);

  int KeyCode() const;
  int GetX() const;
  int GetY() const;
  uint32_t Codepoint() const;

private:
  double time_;
  int type_;
  int argv_[4];
};

class InputEventReceiver
{
public:
  InputEventReceiver();
  ~InputEventReceiver();
  virtual void OnInputEvent(const InputEvent &e) = 0;
};

/**
 * @brief
 * Events related to game 
 * (Not for input event;
 *  input event is processed in InputEvent / InputManager class)
 */
class EventMessage
{
public:
  EventMessage() = default;
  EventMessage(const std::string &name);
  void SetEventName(const std::string &name);
  const std::string &GetEventName() const;

private:
  std::string name_;
};

class EventReceiver
{
public:
  virtual ~EventReceiver();

  void SubscribeTo(const std::string &name);
  void UnsubscribeAll();

  virtual bool OnEvent(const EventMessage& msg);
  friend class EventManager;

private:
  /* to what events it subscribed. */
  std::vector<std::string> subscription_;
};

class EventManager
{
public:
  void Subscribe(EventReceiver& e, const std::string &name);
  void Unsubscribe(EventReceiver& e);
  bool IsSubscribed(EventReceiver& e, const std::string &name);

  /* broadcast event to whole subscriber of it. */
  //static void SendEvent(int event_id);
  static void SendEvent(const std::string& event_name);
  static void SendEvent(const EventMessage &msg);
  static void SendEvent(EventMessage &&msg);

  /* e.g. Set input event handler */
  static void Initialize();

  /* Flush all events for each rendering.
   * Use */
  static void Flush();

  static EventManager& getInstance();
private:
  EventManager();
  ~EventManager();

  /* subscribers are stored in it */
  std::map<std::string, std::set<EventReceiver*> > event_subscribers_;

  int current_evtidx_;

  friend class EventReceiver;
};

/* @brief simple utility to use delayed events */
class QueuedEventCache
{
public:
  QueuedEventCache();
  void QueueEvent(const std::string &name, float timeout_msec);
  float GetRemainingTime(const std::string& queue_name);
  bool IsQueued(const std::string& queue_name);
  void Update(float delta);
  void SetQueueable(bool allow_queue);

  /* @brief process all queued events regardless of remaining time */
  void FlushAll();

  void DeleteAll();
private:
  struct QueuedEvent
  {
    std::string name;   // event to trigger
    float time_delta;   // remain time of this event
  };
  std::list<QueuedEvent> events_;
  bool allow_queue_;
};

}