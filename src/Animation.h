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
  bool display;             // display: show or hide
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
  uint32_t time_duration;   // original duration time of this loop
  uint32_t time_loopstart;  // time to start when looping
  uint32_t time_eclipsed;   // current tween's eclipsed time
  bool loop;                // should this tween loop?
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
  void AddTweenHideDuration(uint32_t time_delta);
  Tween& GetLastTween();
  const Tween& GetLastTween() const;
  void SetTweenLoopTime(uint32_t time_msec);
  const TweenInfo& GetCurrentTweenInfo() const;
  void Load(SpriteAnimation& from_other);
  void LoadSource(SpriteAnimation& from_other);
  void LoadTween(SpriteAnimation& from_other);
  void LoadTweenCurr(SpriteAnimation& from_other);
  void LoadTweenCurr(TweenInfo& target);

  void GetVertexInfo(VertexInfo* vi);
  void GetDrawInfo(DrawInfo& di);

  void SetSource(float sx, float sy, float sw, float sh);
  void SetAnimatedSource(
    float sx, float sy, float sw, float sh,
    int divx, int divy, int timer, int loop_time);
  void UseAnimatedTexture(bool use_ani_tex);

  void SetPosition(float x, float y);
  void MovePosition(float x, float y);
  void SetSize(float w, float h);
  void SetRGB(float r, float g, float b);
  void SetAlpha(float a);
  void SetScale(float x, float y);
  void SetRotation(float x, float y, float z);
  void SetCenter(float x, float y);

  void Show();
  void Hide();
  bool IsDisplay() const;
private:
  /* dynamic tween object, which is actually used when rendering */
  std::list<Tween> tweens_;

  /* calculated tween information by Tick() / UpdateTween() method. */
  TweenInfo current_tween_;

  /* animated texture (generally for LR2) */
  AnimatedTexture ani_texture_;

  /* whether to use ani_texture_ src coordinate or use vertex coordinate of current_tween. */
  bool use_ani_texture_;

  void UpdateTween();
};

void MakeTween(TweenInfo& ti, const TweenInfo& t1, const TweenInfo& t2, double r,
  int ease_type = TweenTypes::kTweenTypeLinear);

}