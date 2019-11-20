#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Util.h"
#include <iostream>
#include <algorithm>

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), divx_(1), divy_(1), cnt_(1), interval_(0), blending_(1),
    idx_(0), eclipsed_time_(0),
    sx_(0), sy_(0), sw_(1.0f), sh_(1.0f), tex_attribute_(0)
{
}

Sprite::~Sprite()
{
  ResourceManager::UnloadImage(img_);
}

void Sprite::SetImageByPath(const std::string &path)
{
  Image *img = ResourceManager::LoadImage(path);
  if (!img)
    return;
  if (img_)
    ResourceManager::UnloadImage(img_);

  img_ = img;

  /* default image src set */
  sx_ = sy_ = 0.f;
  sw_ = sh_ = 1.f;
}

void Sprite::SetBlend(int blend_mode)
{
  blending_ = blend_mode;
}

void Sprite::ReplaySprite()
{
  eclipsed_time_ = 0;
}

void Sprite::Load(const Metric& metric)
{
  BaseObject::Load(metric);

  if (metric.exist("blend"))
    blending_ = metric.get<int>("blend");

  if (metric.exist("sprite"))
  {
    /* imgname,sx,sy,sw,sh,divx,divy,cycle,timer */
    CommandArgs args(metric.get<std::string>("sprite"), 12);

    SetImageByPath(args.Get<std::string>(0));

    if (!img_)
      return;

    if (args.size() > 4)
    {
      int sx = args.Get<int>(1);
      int sy = args.Get<int>(2);
      int sw = args.Get<int>(3);
      int sh = args.Get<int>(4);

      sx_ = sx / (float)img_->get_width();
      sy_ = sy / (float)img_->get_height();
      if (sw < 0) sw_ = 1.0f;
      else sw_ = sw / (float)img_->get_width();
      if (sh < 0) sh_ = 1.0f;
      else sh_ = sh / (float)img_->get_height();
    }

    if (args.size() > 7)
    {
      int divx = args.Get<int>(5);
      int divy = args.Get<int>(6);

      divx_ = divx > 0 ? divx : 1;
      divy_ = divy > 0 ? divy : 1;
      cnt_ = divx_ * divy_;
      interval_ = args.Get<int>(7);
    }

    if (args.size() > 8)
    {
      // Register event : Sprite restarting
      AddCommand("LR" + args.Get<std::string>(8) + "On", "replay");
    }
  }
}

void Sprite::doRender()
{
  // If hide, then not draw
  if (!img_ || !IsVisible())
    return;

  Graphic::SetTextureId(img_->get_texture_ID());
  switch (blending_)
  {
  case 0:
    Graphic::SetBlendMode(GL_ZERO);
    break;
  case 1:
    Graphic::SetBlendMode(GL_ONE_MINUS_SRC_ALPHA);
    break;
  case 2:
    Graphic::SetBlendMode(GL_ONE);
    break;
  }

  const DrawProperty &ti = current_prop_;

  float x1, y1, x2, y2;

  x1 = ti.x;
  y1 = ti.y;
  x2 = x1 + ti.w;
  y2 = y1 + ti.h;

#if 0
  // for predefined src width / height (-1 means use whole texture)
  if (ti.sw == -1) sx1 = 0.0, sx2 = 1.0;
  if (ti.sh == -1) sy1 = 0.0, sy2 = 1.0;
#endif

  VertexInfo* vi_ = Graphic::get_vertex_buffer();
  vi_[0].x = x1;
  vi_[0].y = y1;
  vi_[0].z = 0;
  vi_[0].r = ti.r;
  vi_[0].g = ti.g;
  vi_[0].b = ti.b;
  vi_[0].a = ti.aTL;

  vi_[1].x = x2;
  vi_[1].y = y1;
  vi_[1].z = 0;
  vi_[1].r = ti.r;
  vi_[1].g = ti.g;
  vi_[1].b = ti.b;
  vi_[1].a = ti.aBL;

  vi_[2].x = x2;
  vi_[2].y = y2;
  vi_[2].z = 0;
  vi_[2].r = ti.r;
  vi_[2].g = ti.g;
  vi_[2].b = ti.b;
  vi_[2].a = ti.aBR;

  vi_[3].x = x1;
  vi_[3].y = y2;
  vi_[3].z = 0;
  vi_[3].r = ti.r;
  vi_[3].g = ti.g;
  vi_[3].b = ti.b;
  vi_[3].a = ti.aTR;


  float sw = sw_ / divx_;
  float sh = sh_ / divy_;
  float sx = sx_ + sw * (idx_ % divx_);
  float sy = sy_ + sh * (idx_ / divx_ % divy_);

  vi_[0].sx = sx;
  vi_[0].sy = sy;
  vi_[1].sx = sx + sw;
  vi_[1].sy = sy;
  vi_[2].sx = sx + sw;
  vi_[2].sy = sy + sh;
  vi_[3].sx = sx;
  vi_[3].sy = sy + sh;

  Graphic::RenderQuad();
}

// milisecond
void Sprite::doUpdate(float delta)
{
  // update sprite info
  if (interval_ > 0)
  {
    eclipsed_time_ += delta;
    idx_ = eclipsed_time_ * divx_ * divy_ / interval_ % cnt_;
    eclipsed_time_ = fmod(eclipsed_time_, interval_);
  }
}

CommandFnMap& Sprite::GetCommandFnMap() const
{
  static CommandFnMap cmdfnmap;
  if (cmdfnmap.empty())
  {
    cmdfnmap = BaseObject::GetCommandFnMap();
    cmdfnmap["blend"] = [](void *o, CommandArgs& args) {
      static_cast<Sprite*>(o)->SetBlend(args.Get<int>(0));
    };
    cmdfnmap["replay"] = [](void *o, CommandArgs& args) {
      static_cast<Sprite*>(o)->ReplaySprite();
    };
  }
  return cmdfnmap;
}

}