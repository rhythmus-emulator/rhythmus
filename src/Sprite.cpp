#include "Sprite.h"
#include "Game.h"
#include "SceneManager.h" // to use scene timer
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

const DrawInfo& Sprite::get_drawinfo() const
{
  return di_;
}

DrawInfo& Sprite::get_drawinfo()
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
  invalidate_drawinfo_ = true;
}

void Sprite::SetSize(int w, int h)
{
  // ani_.StopAnimation();
  ani_.SetSize(w, h);
  invalidate_drawinfo_ = true;
}

void Sprite::SetAlpha(float a)
{
  ani_.SetAlpha(a);
  invalidate_drawinfo_ = true;
}

void Sprite::SetRGB(float r, float g, float b)
{
  ani_.SetRGB(r, g, b);
  invalidate_drawinfo_ = true;
}

void Sprite::SetScale(float x, float y)
{
  ani_.SetScale(x, y);
  invalidate_drawinfo_ = true;
}

void Sprite::SetRotation(float x, float y, float z)
{
  ani_.SetRotation(x, y, z);
  invalidate_drawinfo_ = true;
}

void Sprite::SetCenter(float x, float y)
{
  ani_.SetCenter(x, y);
  invalidate_drawinfo_ = true;
}

void Sprite::Hide()
{
  ani_.Hide();
  invalidate_drawinfo_ = true;
}

void Sprite::Show()
{
  ani_.Show();
  invalidate_drawinfo_ = true;
}

const std::string& Sprite::get_name() const
{
  return sprite_name_;
}

void Sprite::Render()
{
  // If hide, then not draw
  if (!ani_.IsDisplay())
    return;

  // Render with given frame
  if (img_)
    glBindTexture(GL_TEXTURE_2D, img_->get_texture_ID());
  Graphic::RenderQuad(di_);
}

void Sprite::Update()
{
  // Before Tick(), check is there any motion
  // If not, we don't need to update DrawInfo.
  bool update_drawinfo = ani_.IsActive() || invalidate_drawinfo_;
  invalidate_drawinfo_ = false;

  // always Tick() to update tween
  ani_.Tick(SceneManager::getInstance().GetSceneTimer().GetDeltaTimeInMillisecond());

  if (update_drawinfo)
    ani_.GetDrawInfo(di_);
}

}