#include "Timer.h"
#include <GLFW/glfw3.h>
#include <atomic>

namespace rhythmus
{

constexpr double TickRateUpdateRate = 0.2;

struct {
  std::atomic<double> game_current_time_, game_prev_time_;
} GlobalTimerContext;

/**
 * @brief Constructor for general timer
 */
Timer::Timer()
  : start_time_(Timer::GetGameTime()), last_time_(start_time_),
    tick_rate_(0), event_interval_(0), event_next_tick_(0), event_loop_(false)
{
}

Timer::~Timer()
{
}

/**
 * @brief Update last time(rate) of timer & call callback functions if necessary.
 */
void Timer::Tick()
{
  // update tick rate
  double new_last_time = Timer::GetGameTime();
  double delta = new_last_time - last_time_;
  last_time_ = new_last_time;
  tick_rate_ = tick_rate_ * (1 - TickRateUpdateRate) + (1 / delta) * TickRateUpdateRate;

  // call callback function
  if (event_interval_ > 0)
  {
    event_next_tick_ -= delta;
    if (event_next_tick_ <= 0)
    {
      OnEvent();
      if (event_loop_)
        event_next_tick_ += event_interval_;
    }
  }

  OnTick(delta);
}

/* Timer automatically starts when this method is called. */
void Timer::SetEventInterval(double interval_second, bool loop)
{
  event_interval_ = interval_second;
  event_loop_ = loop;
  RestartEvent();
}

/* Implement on your own. */
void Timer::OnEvent()
{
}

/* Implement on your own. */
void Timer::OnTick(double delta)
{
}

void Timer::RestartEvent()
{
  event_next_tick_ = event_interval_;
}

void Timer::ClearEvent()
{
  event_interval_ = 0;
}

double Timer::GetTickRate()
{
  return tick_rate_;
}

double Timer::GetTime()
{
  return GlobalTimerContext.game_current_time_.load() - start_time_;
}

uint32_t Timer::GetTimeInMillisecond()
{
  return static_cast<uint32_t>(GetTime() * 1000);
}

double Timer::GetUncachedGameTime()
{
  return glfwGetTime();
}

double Timer::GetGameTime()
{
  return GlobalTimerContext.game_current_time_.load();
}

uint32_t Timer::GetGameTimeInMillisecond()
{
  return static_cast<uint32_t>(GetGameTime() * 1000);
}

double Timer::GetGameTimeDelta()
{
  return GlobalTimerContext.game_current_time_.load() -
    GlobalTimerContext.game_prev_time_.load();
}

uint32_t Timer::GetGameTimeDeltaInMillisecond()
{
  return static_cast<uint32_t>(GetGameTimeDelta() * 1000);
}

/* Should be thread-safe. Use atomic store here. */
void Timer::Update()
{
  GlobalTimerContext.game_prev_time_ = GlobalTimerContext.game_current_time_.load();
  GlobalTimerContext.game_current_time_ = glfwGetTime();
}

void Timer::Initialize()
{
  glfwSetTime(0);
  GlobalTimerContext.game_prev_time_ = 0;
  GlobalTimerContext.game_current_time_ = 0;
}

}