#include "Event.h"

namespace rhythmus
{

EventMessage::EventMessage(int id)
  : id_(id) {}

void EventMessage::SetDesc(const std::string& desc) { desc_ = desc; }
const std::string& EventMessage::GetDesc() const { return desc_; }
void EventMessage::SetEventID(int id) { id_ = id; }
int EventMessage::GetEventID() const { return id_; }



EventReceiver::~EventReceiver()
{
  UnsubscribeAll();
}

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

void EventReceiver::OnEvent(const EventMessage& msg)
{
  // Fill oneself
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

void EventManager::SendEvent(const EventMessage &msg)
{
  auto &evtlist = getInstance().event_subscribers_[msg.GetEventID()];
  for (auto* recv : evtlist)
  {
    recv->OnEvent(msg);
  }
}

EventManager& EventManager::getInstance()
{
  static EventManager em;
  return em;
}

}