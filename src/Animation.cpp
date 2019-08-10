#include "Animation.h"

namespace rhythmus
{

SpriteAnimation::SpriteAnimation()
{
  memset(&current_tween_, 0, sizeof(current_tween_));
  current_tween_.sw = current_tween_.sh = 1.0f;
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);

  ani_texture_.cnt = 1;
  ani_texture_.divx = 1;
  ani_texture_.divy = 1;
  ani_texture_.interval = 0;
  ani_texture_.idx = 0;
  ani_texture_.eclipsed_time = 0;
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
  // Update AnimatedTexture (src)
  if (ani_texture_.interval > 0)
  {
    ani_texture_.eclipsed_time += delta_ms;
    ani_texture_.idx = (ani_texture_.idx + ani_texture_.eclipsed_time / ani_texture_.interval) % ani_texture_.cnt;
    ani_texture_.eclipsed_time %= ani_texture_.interval;
  }

  // Update tween time (dst)
  while (IsActive() && delta_ms > 0)
  {
    Tween &t = tweens_.front();
    if (delta_ms + t.time_eclipsed >= t.time_original)  // in case of last tween
    {
      delta_ms -= t.time_original - t.time_eclipsed;

      // loop tween by push it back.
      if (t.loop)
      {
        t.time_eclipsed = 0;
        tweens_.push_back(t);
      }

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

Tween& SpriteAnimation::GetLastTween()
{
  return tweens_.back();
}

const Tween& SpriteAnimation::GetLastTween() const
{
  return tweens_.back();
}

const TweenInfo& SpriteAnimation::GetCurrentTweenInfo() const
{
  return current_tween_;
}

void SpriteAnimation::GetVertexInfo(VertexInfo* vi)
{
  const TweenInfo &ti = GetCurrentTweenInfo();

  float x1, y1, x2, y2, sx1, sx2, sy1, sy2;
  // window width / height (for ratio)
  float ww = 1.0f, wh = 1.0f;
  // image width / height (for ratio)
  float iw = 1.0f, ih = 1.0f;

  x1 = 0; // ti.x / ww;
  y1 = 0; // ti.y / wh;
  x2 = x1 + ti.w / ww;
  y2 = y1 + ti.h / wh;
  sx1 = ti.sx / iw;
  sy1 = ti.sy / ih;
  sx2 = sx1 + ti.sw / iw;
  sy2 = sy1 + ti.sh / ih;

  // for predefined src width / height (-1 means use whole texture)
  if (ti.sw == -1) sx1 = 0.0, sx2 = 1.0;
  if (ti.sh == -1) sy1 = 0.0, sy2 = 1.0;

  vi[0].x = x1;
  vi[0].y = y1;
  vi[0].z = 0;
  vi[0].sx = sx1;
  vi[0].sy = sy2;
  vi[0].r = ti.r;
  vi[0].g = ti.g;
  vi[0].b = ti.b;
  vi[0].a = ti.aTL;

  vi[1].x = x1;
  vi[1].y = y2;
  vi[1].z = 0;
  vi[1].sx = sx1;
  vi[1].sy = sy1;
  vi[1].r = ti.r;
  vi[1].g = ti.g;
  vi[1].b = ti.b;
  vi[1].a = ti.aBL;

  vi[2].x = x2;
  vi[2].y = y2;
  vi[2].z = 0;
  vi[2].sx = sx2;
  vi[2].sy = sy1;
  vi[2].r = ti.r;
  vi[2].g = ti.g;
  vi[2].b = ti.b;
  vi[2].a = ti.aBR;

  vi[3].x = x2;
  vi[3].y = y1;
  vi[3].z = 0;
  vi[3].sx = sx2;
  vi[3].sy = sy2;
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
  TWEEN(pi.rotx) \
  TWEEN(pi.roty) \
  TWEEN(pi.rotz) \
  TWEEN(pi.tx) \
  TWEEN(pi.ty) \
  TWEEN(pi.x) \
  TWEEN(pi.y) \
  TWEEN(pi.sx) \
  TWEEN(pi.sy)

void SpriteAnimation::UpdateTween()
{
  if (!IsActive()) return;

  TweenInfo& ti = current_tween_;
  if (tweens_.size() == 1)
  {
    ti = tweens_.front().ti;
  }
  else
  {
    const Tween &t1 = tweens_.front();
    const Tween &t2 = *std::next(tweens_.begin());
    float r = (float)t1.time_eclipsed / t1.time_original;

    // TODO: use SSE to optimize.
    switch (t1.ease_type)
    {
    case TweenTypes::kTweenTypeLinear:
    {
#define TWEEN(attr) \
  ti.attr = t1.ti.attr * (1 - r) + t2.ti.attr * r;

      TWEEN_ATTRS;

#undef TWEEN
      break;
    }
    case TweenTypes::kTweenTypeEaseIn:
    {
      // use cubic function
      r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.ti.attr * (1 - r) + t2.ti.attr * r;

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
  ti.attr = t1.ti.attr * (1 - r) + t2.ti.attr * r;

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
  ti.attr = t1.ti.attr * (1 - r) + t2.ti.attr * r;

      TWEEN_ATTRS;

#undef TWEEN
      break;
    }
    case TweenTypes::kTweenTypeNone:
    default:
    {
#define TWEEN(attr) \
  ti.attr = t1.ti.attr;

      TWEEN_ATTRS;

#undef TWEEN
      break;
    }
    }
  }

  // If animated src is alive, then previous sx / sy is ignored.
  if (ani_texture_.interval > 0)
  {
    ti.sx = 1.0f / ani_texture_.divx * (ani_texture_.idx % ani_texture_.divx);
    ti.sy = 1.0f / ani_texture_.divy * (ani_texture_.idx / ani_texture_.divx % ani_texture_.divy);
    ti.sw = 1.0f / ani_texture_.divx;
    ti.sh = 1.0f / ani_texture_.divy;
  }
}

void SpriteAnimation::SetPosition(float x, float y)
{
  current_tween_.pi.x = x;
  current_tween_.pi.y = y;
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

}