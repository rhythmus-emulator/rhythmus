#pragma once

#include <set>
#include <map>
#include <vector>
#include <list>

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

  kEventSongListLoaded,
  kEventSongListLoadFinished,

  kEventSceneTimeout,
  kEventSceneChange,
  kEventSongSelectChanged,
  kEventSongSelected,
  kEventSongLoadFinished,
  kEventSongStarted,
  kEventPlayStarted,
  kEventPlayAborted,
  kEventCleared,

  kEventLast    /* unused event; just for last index */
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
  void SetParam(int *params, int param_len);
  uint32_t GetTimeInMilisecond() const;
  double GetTime() const;

  /**
   * This methods only called
   * when unsynchronized time with rendering is necessary
   * e.g. Input event
   */
  void SetNewTimeUncached();

  bool IsKeyDown() const;
  bool IsKeyPress() const;
  bool IsKeyUp() const;
  bool IsInput() const;
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

  /* subscribers are stored in it */
  std::map<int, std::set<EventReceiver*> > event_subscribers_;

  /* for conversion: event name to id (not implemented yet) */
  std::map<std::string, int> evtname_to_evtid_;

  int current_evtidx_;

  friend class EventReceiver;
};

/* @brief simple utility to use delayed events */
class QueuedEventCache
{
public:
  void QueueEvent(int event_id, float timeout_msec);
  void QueueEvent(const std::string& queue_name, int event_id, float timeout_msec);
  float GetRemainingTime(const std::string& queue_name);
  bool IsQueued(const std::string& queue_name);
  void Update(float delta);

  /* @brief process all queued events regardless of remaining time */
  void FlushAll();

  void DeleteAll();
private:
  struct QueuedEvent
  {
    std::string queue_name;
    float time_delta;   // remain time of this event
    int event_id;       // event to trigger
  };
  std::list<QueuedEvent> events_;
};

}