#include "Timer.h"
#include "common.h"
#include <GLFW/glfw3.h>

namespace rhythmus
{

constexpr double TickRateUpdateRate = 0.2;

/**
 * @brief Constructor for general timer
 */
Timer::Timer()
  : start_time_(0), last_time_(0),
    delta_(0), tick_rate_(0), timer_started_(false)
{
}

Timer::~Timer()
{
}

void Timer::Start()
{
  last_time_ = start_time_ = Timer::GetUncachedSystemTime();
  delta_ = 0;
  timer_started_ = true;
}

void Timer::Stop()
{
  timer_started_ = false;
}

bool Timer::IsTimerStarted()
{
  return timer_started_;
}

/**
 * @brief Update last time(rate) of timer & call callback functions if necessary.
 */
void Timer::Tick()
{
  if (!timer_started_)
    return;

  // update tick rate
  double new_last_time = Timer::GetUncachedSystemTime();
  delta_ = new_last_time - last_time_;
  last_time_ = new_last_time;
  if (delta_ > 0)
    tick_rate_ = tick_rate_ * (1 - TickRateUpdateRate) + (1 / delta_) * TickRateUpdateRate;
  else
    tick_rate_ = .0;  // inf_
}

double Timer::GetTickRate() const
{
  return tick_rate_;
}

double Timer::GetTime() const
{
  return last_time_ - start_time_;
}

uint32_t Timer::GetTimeInMillisecond() const
{
  return static_cast<uint32_t>(GetTime() * 1000);
}

double Timer::GetDeltaTime() const
{
  return delta_;
}

uint32_t Timer::GetDeltaTimeInMillisecond() const
{
  return static_cast<uint32_t>(GetDeltaTime() * 1000);
}

double Timer::GetUncachedSystemTime()
{
  return glfwGetTime();
}

Timer &Timer::SystemTimer()
{
  static Timer sys_timer;
  return sys_timer;
}

void Timer::Update()
{
  SystemTimer().Tick();
}

void Timer::Initialize()
{
  glfwSetTime(0);
  SystemTimer().Start();
}

void Timer::ClearDelta()
{
  delta_ = 0;
}

}
