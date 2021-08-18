#pragma once

#include "Graphic.h"
#include <string>
#include <vector>
#include <list>
#include <map>

namespace rhythmus
{

class LR2FnArgs;

/** @brief Drawing properties of Object */
struct DrawProperty
{
  Vector4 pos;    // x1, y1, x2, y2
  Vector4 color;  // a, r, g, b
  Vector3 rotate; // rotation
  Point align;    // center(align) of object (.0f ~ .1f)
  Point scale;    // object scale
};

/** @brief Tweens' ease type */
enum EaseTypes
{
  kEaseNone,
  kEaseLinear,
  kEaseIn,
  kEaseOut,
  kEaseInOut,
  kEaseInOutBack,
};

void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  float r, int ease_type);

/**
 * @brief
 * Declaration for state of current tween
 * State includes:
 * - Drawing state (need to be animated)
 * - Tween itself information: Easetype, Time (duration), ...
 * - Loop
 * - Tween ease type
 * - Event to be called (kind of triggering)
 *
 * @warn
 * If frame remains, then tweening command won't be executed.
 */
struct AnimationFrame
{
  DrawProperty draw_prop;
  double time;              // timepoint of this frame
  int ease_type;            // tween ease type
};

class Animation
{
public:
  Animation();
  void Clear();
  void DuplicateFrame(double delta);
  void AddFrame(const AnimationFrame& frame);
  void AddFrame(AnimationFrame&& frame);
  void AddFrame(const DrawProperty& draw_prop, double time, int ease_type);
  void AddFrame(const LR2FnArgs& arg);
  void Update(double ms);
  void SetLoop(unsigned repeat_start_time);
  void SetEase(int ease);
  void DeleteLoop();
  void Replay();
  void Play();
  void Pause();
  void Stop();
  void HurryTween();

  void GetDrawProperty(DrawProperty& out);
  const DrawProperty& LastFrame() const;
  DrawProperty& LastFrame();
  double GetLastTime() const;
  int GetFrame() const;
  size_t frame_size() const;
  bool is_empty() const;
  bool is_playing() const;
  bool is_finished() const;

  // used if currently tweening.
  // ex) if 500~1500 tween (not starting from zero),
  //     then if tween time is
  // 50ms   : false
  // 700ms  : true
  // 1600ms : false (actually, animation is already finished)
  bool is_tweening() const;

private:
  std::vector<AnimationFrame> frames_;  // frame by timeline
  double time_;               // time for total frame timeline
  int frame_;                 // current frame. if -1, then time is yet to first frame. (empty frame)
  bool paused_;
  bool repeat_;
  double repeat_start_time_;
  bool is_timeline_finished_;
};

class QueuedAnimation {
public:
  QueuedAnimation();
  void Play(const DrawProperty& init_state);

  void SetCommand(const std::string& command);

  bool is_empty() const;

private:
  std::list<AnimationFrame> frames_;  // frame by queued command  DrawProperty init_state_;   // initial state of tween morphing (command)
  DrawProperty init_state_;   // initial state of tween morphing (command)
  double time_;               // time for current frame
  bool paused_;
};

}