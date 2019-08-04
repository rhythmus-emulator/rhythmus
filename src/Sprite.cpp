#include "Sprite.h"
#include "Game.h"
#include "Timer.h"
#include <iostream>

namespace rhythmus
{

Sprite::Sprite()
{
  memset(&di_, 0, sizeof(di_));
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

void Sprite::SetAlpha(float a)
{
  ani_.SetAlpha(a);
}

void Sprite::SetRGB(float r, float g, float b)
{
  ani_.SetRGB(r, g, b);
}

void Sprite::Render()
{
  // check animation is now in active
  // if active, call Tick() and invalidate new frame
  if (ani_.IsActive())
    ani_.Tick(Timer::GetGameTimeDeltaInMillisecond());
  ani_.GetDrawInfo(di_);

  // Render with given frame
  if (img_)
    glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());
  Graphic::RenderQuad(di_);
}

}