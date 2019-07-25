#include "Sprite.h"
#include "Game.h"
#include <iostream>

namespace rhythmus
{

Sprite::Sprite()
{
  memset(&frame_, 0, sizeof(frame_));
  frame_.src.w = frame_.src.h = -1;
  memset(&rframe_, 0, sizeof(rframe_));
  memset(&hlsl_vert_, 0, sizeof(hlsl_vert_));
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

  hlsl_vert_[0].r = hlsl_vert_[0].g = hlsl_vert_[0].b = hlsl_vert_[0].a = 1.0f;
  hlsl_vert_[1].r = hlsl_vert_[1].g = hlsl_vert_[1].b = hlsl_vert_[1].a = 1.0f;
  hlsl_vert_[2].r = hlsl_vert_[2].g = hlsl_vert_[2].b = hlsl_vert_[2].a = 1.0f;
  hlsl_vert_[3].r = hlsl_vert_[3].g = hlsl_vert_[3].b = hlsl_vert_[3].a = 1.0f;
}

void Sprite::SetPos(int x, int y)
{
  frame_.dstpos.x = x;
  frame_.dstpos.y = y;
  InvalidateRFrame();
}

void Sprite::SetSize(int w, int h)
{
  frame_.dst.w = w;
  frame_.dst.h = h;
  InvalidateRFrame();
}

// TODO: optimize with SSE
void Sprite::InvalidateRFrame()
{
  const float ww = 1.0f; //Game::getInstance().get_window_width();
  const float wh = 1.0f; //Game::getInstance().get_window_height();
  const float iw = img_->get_width();
  const float ih = img_->get_height();

  rframe_.sx = frame_.srcpos.x / iw;
  rframe_.sy = frame_.srcpos.y / ih;
  rframe_.sx2 = rframe_.sx + frame_.src.w / iw;
  rframe_.sy2 = rframe_.sy + frame_.src.h / ih;
  rframe_.dx = frame_.dstpos.x / ww;
  rframe_.dy = frame_.dstpos.y / wh;
  rframe_.dx2 = rframe_.dx + frame_.dst.w / ww;
  rframe_.dy2 = rframe_.dy + frame_.dst.h / wh;

  if (frame_.src.w == -1) rframe_.sx = 0.0, rframe_.sx2 = 1.0;
  if (frame_.src.h == -1) rframe_.sy = 0.0, rframe_.sy2 = 1.0;

  hlsl_vert_[0].x = rframe_.dx;
  hlsl_vert_[0].y = rframe_.dy;
  hlsl_vert_[0].z = 0;
  hlsl_vert_[0].sx = rframe_.sx;
  hlsl_vert_[0].sy = rframe_.sy2;

  hlsl_vert_[1].x = rframe_.dx;
  hlsl_vert_[1].y = rframe_.dy2;
  hlsl_vert_[1].z = 0;
  hlsl_vert_[1].sx = rframe_.sx;
  hlsl_vert_[1].sy = rframe_.sy;

  hlsl_vert_[2].x = rframe_.dx2;
  hlsl_vert_[2].y = rframe_.dy2;
  hlsl_vert_[2].z = 0;
  hlsl_vert_[2].sx = rframe_.sx2;
  hlsl_vert_[2].sy = rframe_.sy;

  hlsl_vert_[3].x = rframe_.dx2;
  hlsl_vert_[3].y = rframe_.dy;
  hlsl_vert_[3].z = 0;
  hlsl_vert_[3].sx = rframe_.sx2;
  hlsl_vert_[3].sy = rframe_.sy2;
}

void Sprite::Render()
{
  // check animation is now in active
  // if active, call Tick() and invalidate new frame
  if (ani_.IsActive())
  {
    ani_.Tick(0); //  TODO: implement timer
    ani_.GetFrame(frame_);
    InvalidateRFrame();
  }

  // render with given frame
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());
  Graphic::RenderQuad(hlsl_vert_);
}

}