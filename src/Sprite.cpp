#include "Sprite.h"
#include "Game.h"
#include "Timer.h"
#include <iostream>

namespace rhythmus
{

Sprite::Sprite()
{
  memset(&di_, 0, sizeof(di_));

  di_.vi[0].r = di_.vi[0].g = di_.vi[0].b = di_.vi[0].a = 1.0f;
  di_.vi[1].r = di_.vi[1].g = di_.vi[1].b = di_.vi[1].a = 1.0f;
  di_.vi[2].r = di_.vi[2].g = di_.vi[2].b = di_.vi[2].a = 1.0f;
  di_.vi[3].r = di_.vi[3].g = di_.vi[3].b = di_.vi[3].a = 1.0f;

  // XXX: test animation
  ani_.AddTween({ {
      10, 10, 100, 100,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    }, 1000, 0, true, TweenTypes::kTweenTypeEaseOut
    });
  ani_.AddTween({ {
      100, 100, 110, 110,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    }, 1500, 0, true, TweenTypes::kTweenTypeEaseOut
    });
}

Sprite::~Sprite()
{
}

SpriteAnimation& Sprite::get_animation()
{
  return ani_;
}

void Sprite::SetImage(ImageAuto img)
{
  img_ = img;
}

void Sprite::SetPos(int x, int y)
{
  // ani_.StopAnimation();
  ani_.SetPosition(x, y);
}

void Sprite::SetSize(int w, int h)
{
  // ani_.StopAnimation();
  ani_.SetSize(w, h);
}

void Sprite::Render()
{
  // check animation is now in active
  // if active, call Tick() and invalidate new frame
  if (ani_.IsActive())
  {
    ani_.Tick(Timer::GetGameTimeDeltaInMillisecond());
    ani_.GetDrawInfo(di_);
  }

  // Render with given frame
  glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());
  Graphic::RenderQuad(di_);
}

}