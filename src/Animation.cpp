#include "Animation.h"
#include "Logger.h"
#include "ScriptLR2.h"
#include "common.h"

namespace rhythmus
{

#define TWEEN_ATTRS \
  TWEEN(pos) \
  TWEEN(color) \
  TWEEN(rotate) \
  TWEEN(align) \
  TWEEN(scale)


void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  float r, int ease_type)
{
  switch (ease_type)
  {
  case EaseTypes::kEaseLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseOut:
  {
    // use cubic function
    r = 1 - r;
    r = r * r * r;
    r = 1 - r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseInOut:
  {
    // use cubic function
    r = 2 * r - 1;
    r = r * r * r;
    r = 0.5f + r / 2;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1.0f - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseNone:
  default:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  }
}

// ---------------------------------------------------------------- class Tween

Animation::Animation()
  : time_(0), frame_(-1),
    paused_(false), repeat_(false), repeat_start_time_(0),
    is_timeline_finished_(false)
{
}

void Animation::Clear()
{
  frames_.clear();
}

void Animation::DuplicateFrame(double delta)
{
  if (frames_.empty()) return;
  if (delta > 0) frames_.push_back(frames_.back());
  frames_.back().time += delta;
}

void Animation::AddFrame(const AnimationFrame &frame)
{
  if (!frames_.empty() && frames_.back().time >= frame.time)
    frames_.back() = frame;
  else
    frames_.push_back(frame);
}

void Animation::AddFrame(AnimationFrame &&frame)
{
  if (!frames_.empty() && frames_.back().time >= frame.time)
    frames_.back() = frame;
  else
    frames_.emplace_back(frame);
}

void Animation::AddFrame(const DrawProperty &draw_prop, double time, int ease_type)
{
  if (!frames_.empty() && frames_.back().time >= time) {
    /* XXX: incomplete, time may be small then previous frame.
     * need to use ASSERT? */
    frames_.back().time = time;
    frames_.back().draw_prop = draw_prop;
    frames_.back().ease_type = ease_type;
  }
  else
    frames_.emplace_back(AnimationFrame{ draw_prop, time, ease_type });
}

void Animation::AddFrame(const LR2FnArgs& arg)
{
  int time = arg.get_int(2);
  int x = arg.get_int(3);
  int y = arg.get_int(4);
  int w = arg.get_int(5);
  int h = arg.get_int(6);
  int lr2acc = arg.get_int(7);
  int a = arg.get_int(8);
  int r = arg.get_int(9);
  int g = arg.get_int(10);
  int b = arg.get_int(11);
  //int blend = arg.get_int(12);
  //int filter = arg.get_int(13);
  int angle = arg.get_int(14);
  //int center = arg.get_int(15);
  //int loop = arg.get_int(16);
  int acc = 0;
  // timer/op code is ignored here.

  // set attributes
  DrawProperty f;
  f.pos = Vector4{ x, y, x + w, y + h };
  f.color = Vector4{ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
  f.rotate = Vector3{ 0.0f, 0.0f, glm::radians((float)angle) };
  f.scale = Vector2{ 1.0f, 1.0f };
  memset(&f.align, 0, sizeof(f.align));

  switch (lr2acc)
  {
  case 0:
    acc = EaseTypes::kEaseLinear;
    break;
  case 1:
    acc = EaseTypes::kEaseIn;
    break;
  case 2:
    acc = EaseTypes::kEaseOut;
    break;
  case 3:
    acc = EaseTypes::kEaseInOut;
    break;
  }

  // add new animation
  AddFrame(f, time, acc);
}

void Animation::Update(double ms)
{
  // don't do anything if empty.
  if (frames_.empty() || paused_ || is_timeline_finished_) return;

  // find out which frame is it in.
  const double last_time = GetLastTime();
  time_ += ms;
  if (repeat_ && time_ > last_time) {
    const double actual_loop_time = last_time - repeat_start_time_;
    if (actual_loop_time <= 0)  // XXX: edge case (no loop actually)
      time_ = last_time;
    else {
      time_ = fmod(time_ - last_time, actual_loop_time) + repeat_start_time_;
      frame_ = -1;
    }
  }
  else if (!repeat_ && time_ > last_time) {
    //ms -= last_time - time_;
    time_ = last_time;
    is_timeline_finished_ = true;
  }
  for (; frame_ < (int)frames_.size() - 1; ++frame_) {
    if (time_ < frames_[frame_ + 1].time)
      break;
  }
}

void Animation::Replay()
{
  Stop();
  Play();
}

void Animation::Play()
{
  paused_ = false;
}

void Animation::Pause()
{
  paused_ = true;
}

void Animation::Stop()
{
  time_ = 0;
  frame_ = -1;
  paused_ = true;
  is_timeline_finished_ = false;
}

void Animation::HurryTween()
{
  frame_ = (int)frames_.size() - 1;
  time_ = GetLastTime();
}

void Animation::GetDrawProperty(DrawProperty &out)
{
  if (!is_timeline_finished_ && !frames_.empty()) {
    if (frame_ < 0) {
      // even animation is not started, returns first frame.
      out = frames_.front().draw_prop;
    }
    else if (frame_ == frames_.size() - 1) {
      // last frame
      out = frames_.back().draw_prop;
    }
    else {
      MakeTween(out,
        frames_[frame_].draw_prop,
        frames_[frame_ + 1].draw_prop,
        (float)((time_ - frames_[frame_].time) / (frames_[frame_ + 1].time - frames_[frame_].time)),
        frames_[frame_].ease_type);
    }
  }
}

void Animation::SetLoop(unsigned repeat_start_time)
{
  repeat_ = true;
  repeat_start_time_ = (double)repeat_start_time;
}

void Animation::SetEase(int ease)
{
  if (is_empty()) return;
  frames_.back().ease_type = ease;
}

void Animation::DeleteLoop()
{
  repeat_ = false;
  repeat_start_time_ = 0;
}

const DrawProperty &Animation::LastFrame() const
{
  return frames_.back().draw_prop;
}

DrawProperty &Animation::LastFrame()
{
  return frames_.back().draw_prop;
}

double Animation::GetLastTime() const
{
  if (frames_.empty()) return 0;
  else return frames_.back().time;
}

int Animation::GetFrame() const
{
  return frame_;
}

size_t Animation::frame_size() const { return frames_.size(); }
bool Animation::is_empty() const { return frames_.empty(); }
bool Animation::is_playing() const { return !paused_ && is_tweening(); }
bool Animation::is_finished() const { return is_timeline_finished_ || frames_.empty(); }

bool Animation::is_tweening() const
{
  return !frames_.empty() && !is_timeline_finished_;
}

// ---------------------------------------------------------------- QueuedAnimation

QueuedAnimation::QueuedAnimation()
  : paused_(true)
{}

void QueuedAnimation::Play(const DrawProperty& init_state)
{
  init_state_ = init_state;
}

void QueuedAnimation::SetCommand(const std::string& command)
{
  /* TODO */
}

bool QueuedAnimation::is_empty() const
{
  return frames_.empty();
}

}
