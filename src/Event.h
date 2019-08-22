#pragma once

#include <set>
#include <map>
#include <vector>

namespace rhythmus
{

enum Events
{
  kEventNone,
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
  EventMessage(int id);

  void SetDesc(const std::string& desc);
  const std::string& GetDesc() const;
  void SetEventID(int id);
  int GetEventID() const;

private:
  int id_;
  std::string desc_;
};

class EventReceiver
{
public:
  virtual ~EventReceiver();

  void SubscribeTo(int id);
  void UnsubscribeAll();

  virtual void OnEvent(const EventMessage& msg);

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

  static EventManager& getInstance();
private:
  /* subscribers are stored in it */
  std::map<int, std::set<EventReceiver*> > event_subscribers_;

  /* for conversion: event name to id (not implemented yet) */
  std::map<std::string, int> evtname_to_evtid_;

  friend class EventReceiver;
};

}