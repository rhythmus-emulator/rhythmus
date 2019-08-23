#pragma once

#include <set>
#include <map>
#include <vector>

namespace rhythmus
{

/**
 * @brief
 * General game event types (including input event)
 */
enum Events
{
  kEventNone,

  kOnKeyDown,
  kOnKeyPress,  /* key event repeating due to pressing */
  kOnKeyUp,
  kOnText,
  kOnCursorMove,
  kOnCursorClick,
  kOnJoystick,

  kEventSceneChanged,
  kEventSongSelectChanged,
  kEventSongSelected,
  kEventSongStarted,
  kEventPlayStarted,
  kEventPlayAborted,
  kEventCleared,
};

class EventMessage
{
public:
  EventMessage(int id = 0, bool time_synced = true);

  void SetDesc(const std::string& desc);
  const std::string& GetDesc() const;
  void SetEventID(int id);
  int GetEventID() const;
  void SetNewTime();

  /**
   * This methods only called
   * when unsynchronized time with rendering is necessary
   * e.g. Input event
   */
  void SetNewTimeUncached();

  bool IsKeyDown() const;
  bool IsKeyUp() const;
  int GetKeycode() const;

private:
  int id_;
  int params_[4];
  std::string desc_;
  double time_;
};

class EventReceiver
{
public:
  virtual ~EventReceiver();

  void SubscribeTo(int id);
  void UnsubscribeAll();

  virtual bool OnEvent(const EventMessage& msg);

private:
  /* to what events it subscribed. */
  std::vector<Events> subscription_;
};

class EventManager
{
public:
  void Subscribe(EventReceiver& e, int event_id);
  void Unsubscribe(EventReceiver& e);
  bool IsSubscribed(EventReceiver& e, int event_id);

  /* broadcast event to whole subscriber of it. */
  static void SendEvent(int event_id);
  static void SendEvent(const EventMessage &msg);
  static void SendEvent(EventMessage &&msg);

  /* e.g. Set input event handler */
  static void Initialize();

  /* Flush all events for each rendering.
   * Use */
  static void Flush();

  static EventManager& getInstance();
private:
  /* subscribers are stored in it */
  std::map<int, std::set<EventReceiver*> > event_subscribers_;

  /* for conversion: event name to id (not implemented yet) */
  std::map<std::string, int> evtname_to_evtid_;

  friend class EventReceiver;
};

}