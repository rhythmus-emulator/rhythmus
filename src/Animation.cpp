#include "Animation.h"
#include "Timer.h"
#include "common.h"

namespace rhythmus
{

SpriteAnimation::SpriteAnimation()
{
  memset(&current_tween_, 0, sizeof(current_tween_));
  current_tween_.sw = current_tween_.sh = 1.0f;
  current_tween_.display = true;
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);

  ani_texture_.cnt = 1;
  ani_texture_.divx = 1;
  ani_texture_.divy = 1;
  ani_texture_.interval = 0;
  ani_texture_.idx = 0;
  ani_texture_.eclipsed_time = 0;
  ani_texture_.sx = ani_texture_.sy = 0.0f;
  ani_texture_.sw = ani_texture_.sh = 1.0f;
  use_ani_texture_ = false;
}

SpriteAnimation::~SpriteAnimation()
{
}

bool SpriteAnimation::IsActive() const
{
  return !tweens_.empty();
}

void SpriteAnimation::Tick(int delta_ms)
{
  if (delta_ms == 0) return;

  // Update AnimatedTexture (src)
  if (ani_texture_.interval > 0)
  {
    ani_texture_.eclipsed_time += delta_ms;
    ani_texture_.idx =
      ani_texture_.eclipsed_time * ani_texture_.divx * ani_texture_.divy
      / ani_texture_.interval % ani_texture_.cnt;
    ani_texture_.eclipsed_time %= ani_texture_.interval;
  }

  // Update tween time (dst)
  while (IsActive() && delta_ms > 0)
  {
    Tween &t = tweens_.front();
    if (delta_ms + t.time_eclipsed >= t.time_duration)  // in case of last tween
    {
      delta_ms -= t.time_duration - t.time_eclipsed;

      // kind of trick: if single tween with loop,
      // It's actually useless. turn off loop attr.
      if (tweens_.size() == 1)
        t.loop = false;

      // loop tween by push it back.
      if (t.loop)
      {
        t.time_eclipsed = t.time_loopstart;
        tweens_.push_back(t);
      }

      // kind of trick: if current tween is last one,
      // Do UpdateTween here. We expect last tween state
      // should be same as current tween in that case.
      if (tweens_.size() == 1)
        current_tween_ = t.ti;

      tweens_.pop_front();
    }
    else
    {
      t.time_eclipsed += delta_ms;
      delta_ms = 0; // actually break loop.
    }
  }

  UpdateTween();
}

void SpriteAnimation::AddTween(const Tween& tween)
{
  tweens_.push_back(tween);
}

void SpriteAnimation::AddTween(float x, float y, float w, float h,
  float r, float g, float b, float a, uint32_t time_delta, bool loop)
{
  tweens_.emplace_back(Tween{
    /* TweenInfo */ {0, 0, w, h, r, g, b,
    a, a, a, a, .0f, .0f, 1.0f, 1.0f,
    /* ProjectionInfo */ {.0f, .0f, .0f, .0f, .0f, x, y, 1.0f, 1.0f },
    true},
    time_delta, 0, 0, loop, TweenTypes::kTweenTypeEaseOut});
}

void SpriteAnimation::AddTweenHideDuration(uint32_t time_delta)
{
  tweens_.emplace_back(Tween{
    /* TweenInfo */ {0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, .0f, .0f, 1.0f, 1.0f,
    /* ProjectionInfo */ {.0f, .0f, .0f, .0f, .0f, 0, 0, 1.0f, 1.0f },
    false},
    time_delta, 0, 0, false, TweenTypes::kTweenTypeNone });
}

Tween& SpriteAnimation::GetLastTween()
{
  return tweens_.back();
}

const Tween& SpriteAnimation::GetLastTween() const
{
  return tweens_.back();
}

/* This method should be called just after all fixed tween is appended. */
void SpriteAnimation::SetTweenLoopTime(uint32_t time_msec)
{
  for (auto& t : tweens_)
  {
    if (time_msec < t.time_duration)
    {
      t.time_loopstart = time_msec;
      t.loop = true;
      time_msec = 0;
    }
    else {
      t.time_loopstart = 0;
      t.loop = false;
      time_msec -= t.time_duration;
    }
  }
}

const TweenInfo& SpriteAnimation::GetCurrentTweenInfo() const
{
  return current_tween_;
}


void SpriteAnimation::Load(SpriteAnimation& from_other)
{
  LoadSource(from_other);
  LoadTween(from_other);
}

void SpriteAnimation::LoadSource(SpriteAnimation& from_other)
{

}

void SpriteAnimation::LoadTween(SpriteAnimation& from_other)
{

}

void SpriteAnimation::LoadTweenCurr(SpriteAnimation& from_other)
{
  LoadTweenCurr(from_other.current_tween_);
}

void SpriteAnimation::LoadTweenCurr(TweenInfo& target)
{
  current_tween_ = target;
}

void SpriteAnimation::GetVertexInfo(VertexInfo* vi)
{
  const TweenInfo &ti = GetCurrentTweenInfo();

  float x1, y1, x2, y2, sx1, sx2, sy1, sy2;

  x1 = ti.x;
  y1 = ti.y;
  x2 = x1 + ti.w;
  y2 = y1 + ti.h;
  sx1 = ti.sx;
  sy1 = ti.sy;
  sx2 = sx1 + ti.sw;
  sy2 = sy1 + ti.sh;

  // for predefined src width / height (-1 means use whole texture)
  if (ti.sw == -1) sx1 = 0.0, sx2 = 1.0;
  if (ti.sh == -1) sy1 = 0.0, sy2 = 1.0;

  vi[0].x = x1;
  vi[0].y = y1;
  vi[0].z = 0;
  vi[0].sx = sx1;
  vi[0].sy = sy1;
  vi[0].r = ti.r;
  vi[0].g = ti.g;
  vi[0].b = ti.b;
  vi[0].a = ti.aTL;

  vi[1].x = x1;
  vi[1].y = y2;
  vi[1].z = 0;
  vi[1].sx = sx1;
  vi[1].sy = sy2;
  vi[1].r = ti.r;
  vi[1].g = ti.g;
  vi[1].b = ti.b;
  vi[1].a = ti.aBL;

  vi[2].x = x2;
  vi[2].y = y2;
  vi[2].z = 0;
  vi[2].sx = sx2;
  vi[2].sy = sy2;
  vi[2].r = ti.r;
  vi[2].g = ti.g;
  vi[2].b = ti.b;
  vi[2].a = ti.aBR;

  vi[3].x = x2;
  vi[3].y = y1;
  vi[3].z = 0;
  vi[3].sx = sx2;
  vi[3].sy = sy1;
  vi[3].r = ti.r;
  vi[3].g = ti.g;
  vi[3].b = ti.b;
  vi[3].a = ti.aTR;
}

void SpriteAnimation::GetDrawInfo(DrawInfo& di)
{
  GetVertexInfo(di.vi);
  di.pi = current_tween_.pi;
}

#define TWEEN_ATTRS \
  TWEEN(x) \
  TWEEN(y) \
  TWEEN(w) \
  TWEEN(h) \
  TWEEN(r) \
  TWEEN(g) \
  TWEEN(b) \
  TWEEN(aTL) \
  TWEEN(aTR) \
  TWEEN(aBR) \
  TWEEN(aBL) \
  TWEEN(sx) \
  TWEEN(sy) \
  TWEEN(sw) \
  TWEEN(sh) \
  TWEEN(pi.rotx) \
  TWEEN(pi.roty) \
  TWEEN(pi.rotz) \
  TWEEN(pi.tx) \
  TWEEN(pi.ty) \
  TWEEN(pi.x) \
  TWEEN(pi.y) \
  TWEEN(pi.sx) \
  TWEEN(pi.sy) \


void MakeTween(TweenInfo& ti, const TweenInfo& t1, const TweenInfo& t2,
  double r, int ease_type)
{
  switch (ease_type)
  {
  case TweenTypes::kTweenTypeLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case TweenTypes::kTweenTypeEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case TweenTypes::kTweenTypeEaseOut:
  {
    // use cubic function
    r = 1 - r;
    r = r * r * r;
    r = 1 - r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case TweenTypes::kTweenTypeEaseInOut:
  {
    // use cubic function
    r = 2 * r - 1;
    r = r * r * r;
    r = 0.5f + r / 2;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case TweenTypes::kTweenTypeNone:
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

void SpriteAnimation::UpdateTween()
{
  TweenInfo& ti = current_tween_;

  // DST calculation start.
  if (IsActive())
  {
    if (tweens_.size() == 1)
    {
      ti = tweens_.front().ti;
    }
    else
    {
      const Tween &t1 = tweens_.front();
      const Tween &t2 = *std::next(tweens_.begin());
      ti.display = t1.ti.display;

      // If not display, we don't need to calculate further away.
      if (ti.display)
      {
        float r = (float)t1.time_eclipsed / t1.time_duration;
        MakeTween(ti, t1.ti, t2.ti, r, t1.ease_type);
      }
    }
  }

  // SRC calculation start
  // If use_ani_texture_ set,
  // it uses animated SRC info instead of Vertex SRC info.
  if (use_ani_texture_)
  {
    if (ani_texture_.interval > 0)
    {
      ti.sw = ani_texture_.sw / ani_texture_.divx;
      ti.sh = ani_texture_.sh / ani_texture_.divy;
      ti.sx = ani_texture_.sx + ti.sw * (ani_texture_.idx % ani_texture_.divx);
      ti.sy = ani_texture_.sy + ti.sh * (ani_texture_.idx / ani_texture_.divx % ani_texture_.divy);
    }
    else
    {
      ti.sx = ani_texture_.sx;
      ti.sy = ani_texture_.sy;
      ti.sw = ani_texture_.sw;
      ti.sh = ani_texture_.sh;
    }
  }
}

void SpriteAnimation::SetSource(float sx, float sy, float sw, float sh)
{
  ani_texture_.cnt = 1;
  ani_texture_.divx = 1;
  ani_texture_.divy = 1;
  ani_texture_.eclipsed_time = 0;
  ani_texture_.idx = 0;
  ani_texture_.interval = 0;
  ani_texture_.sx = sx;
  ani_texture_.sy = sy;
  ani_texture_.sw = sw;
  ani_texture_.sh = sh;
}

void SpriteAnimation::SetAnimatedSource(
  float sx, float sy, float sw, float sh,
  int divx, int divy, int timer, int loop_time)
{
  ani_texture_.cnt = divx * divy;
  ani_texture_.divx = divx;
  ani_texture_.divy = divy;
  ani_texture_.eclipsed_time = 0;
  ani_texture_.idx = 0;
  ani_texture_.interval = loop_time;
  ani_texture_.sx = sx;
  ani_texture_.sy = sy;
  ani_texture_.sw = sw;
  ani_texture_.sh = sh;
}

void SpriteAnimation::UseAnimatedTexture(bool use_ani_tex)
{
  use_ani_texture_ = use_ani_tex;
}

void SpriteAnimation::SetPosition(float x, float y)
{
  current_tween_.pi.x = x;
  current_tween_.pi.y = y;
}

void SpriteAnimation::MovePosition(float x, float y)
{
  current_tween_.pi.x += x;
  current_tween_.pi.y += y;
}

void SpriteAnimation::SetSize(float w, float h)
{
  current_tween_.w = w;
  current_tween_.h = h;
}

void SpriteAnimation::SetAlpha(float a)
{
  current_tween_.aBL =
    current_tween_.aBR =
    current_tween_.aTL =
    current_tween_.aTR = a;
}

void SpriteAnimation::SetRGB(float r, float g, float b)
{
  current_tween_.r = r;
  current_tween_.g = g;
  current_tween_.b = b;
}

void SpriteAnimation::SetScale(float x, float y)
{
  current_tween_.pi.sx = x;
  current_tween_.pi.sy = y;
}

void SpriteAnimation::SetRotation(float x, float y, float z)
{
  current_tween_.pi.rotx = x;
  current_tween_.pi.roty = y;
  current_tween_.pi.rotz = z;
}

void SpriteAnimation::SetCenter(float x, float y)
{
  current_tween_.pi.tx = x;
  current_tween_.pi.ty = y;
}

void SpriteAnimation::Show()
{
  current_tween_.display = true;
}

void SpriteAnimation::Hide()
{
  current_tween_.display = false;
}

bool SpriteAnimation::IsDisplay() const
{
  return current_tween_.display &&
    current_tween_.aBL > 0 &&
    current_tween_.aBR > 0 &&
    current_tween_.aTL > 0 &&
    current_tween_.aTR > 0;
}

}
