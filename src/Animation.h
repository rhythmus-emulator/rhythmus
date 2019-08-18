#pragma once

#include "Graphic.h"
#include <list>

namespace rhythmus
{

struct TweenInfo
{
  float x, y, w, h;         // general object position
  float r, g, b;            // alpha masking
  float aTL, aTR, aBR, aBL; // alpha fade
  float sx, sy, sw, sh;     // texture src crop
  ProjectionInfo pi;
};

enum TweenTypes
{
  kTweenTypeNone,
  kTweenTypeLinear,
  kTweenTypeEaseIn,
  kTweenTypeEaseOut,
  kTweenTypeEaseInOut,
  kTweenTypeEaseInOutBack,
};

struct Tween
{
  TweenInfo ti;
  uint32_t time_original;
  uint32_t time_eclipsed;
  bool loop;
  int ease_type;            // tween ease type
};

struct AnimatedTexture
{
  int divx, divy, cnt;
  int interval;
  int idx, eclipsed_time;
  float sx, sy, sw, sh;     // texture src crop
};

class SpriteAnimation
{
public:
  SpriteAnimation();
  ~SpriteAnimation();
  bool IsActive() const;
  void Tick(int delta_ms);
  void AddTween(const Tween& tween);
  void AddTween(float x, float y, float w, float h,
    float r, float g, float b, float a, uint32_t time_delta, bool loop);
  Tween& GetLastTween();
  const Tween& GetLastTween() const;
  const TweenInfo& GetCurrentTweenInfo() const;
  void GetVertexInfo(VertexInfo* vi);
  void GetDrawInfo(DrawInfo& di);

  void SetSource(float sx, float sy, float sw, float sh);
  void SetAnimatedSource(
    float sx, float sy, float sw, float sh,
    int divx, int divy, int timer, int loop_time);
  void UseAnimatedTexture(bool use_ani_tex);

  void SetPosition(float x, float y);
  void SetSize(float w, float h);
  void SetRGB(float r, float g, float b);
  void SetAlpha(float a);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetCenter(float x, float y);
private:
  std::list<Tween> tweens_;
  TweenInfo current_tween_;
  AnimatedTexture ani_texture_;
  bool use_ani_texture_;

  void UpdateTween();
};

}