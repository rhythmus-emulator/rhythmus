#pragma once

#include <stdint.h>
#include <vector>
#include <functional>

namespace rhythmus
{

/**
 * @brief
 * Timer with calculating TickRate & with status.
 */
class Timer
{
public:
  Timer();
  ~Timer();
  double GetTime() const;
  uint32_t GetTimeInMillisecond() const;
  double GetDeltaTime() const;
  uint32_t GetDeltaTimeInMillisecond() const;
  void Start();
  void Stop();
  bool IsTimerStarted();
  void Tick();
  double GetTickRate() const;

  static Timer &SystemTimer();
  static double GetUncachedSystemTime();
  static void Initialize();
  static void Update();

private:
  double start_time_;
  double last_time_;
  double delta_;
  double tick_rate_;
  bool timer_started_;
};

}