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

const DrawInfo& Sprite::get_drawinfo()
{
  return di_;
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

void Sprite::SetScale(float x, float y)
{
  ani_.SetScale(x, y);
}

void Sprite::SetRotation(float x, float y, float z)
{
  ani_.SetRotation(x, y, z);
}

void Sprite::SetCenter(float x, float y)
{
  ani_.SetCenter(x, y);
}

void Sprite::Render()
{
  // Render with given frame
  if (img_)
    glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());
  Graphic::RenderQuad(di_);
}

void Sprite::Update()
{
  // check animation is now in active
  // if active, call Tick() and invalidate new frame
  if (ani_.IsActive())
    ani_.Tick(Timer::GetGameTimeDeltaInMillisecond());
  ani_.GetDrawInfo(di_);
}

}