#include "Sprite.h"
#include "Game.h"
#include "Timer.h"
#include <iostream>

namespace rhythmus
{

Sprite::Sprite()
{
  memset(&di_, 0, sizeof(di_));

  // XXX: test animation
  ani_.AddTween({ {
      0, 0, 100, 100,
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, 0.0f, 0.0f, 50, 50, 100, 100
    }, 1000, 0, true, TweenTypes::kTweenTypeEaseOut
    });
  ani_.AddTween({ {
      0, 0, 110, 110,
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 1.0f,
      0.0f, glm::radians(90.0f), 0.0f, 55, 55, 100, 200
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