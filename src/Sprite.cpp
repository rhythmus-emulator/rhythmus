#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Util.h"
#include <iostream>
#include <algorithm>

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), img_owned_(true),
    divx_(1), divy_(1), cnt_(1), interval_(0),
    idx_(0), eclipsed_time_(0),
    sx_(0), sy_(0), sw_(1.0f), sh_(1.0f),
    source_x(0), source_y(0), source_width(0), source_height(0),
    tex_attribute_(0)
{
}

Sprite::~Sprite()
{
  if (img_ && img_owned_)
    ResourceManager::UnloadImage(img_);
}

void Sprite::SetImageByPath(const std::string &path)
{
  Image *img = ResourceManager::LoadImage(path);
  if (!img)
    return;
  if (img_ && img_owned_)
    ResourceManager::UnloadImage(img_);

  img_ = img;
  img_owned_ = true;

  /* default image src set */
  sx_ = sy_ = 0.f;
  sw_ = sh_ = 1.f;
}

void Sprite::SetImage(Image *img)
{
  if (img_ && img_owned_)
    ResourceManager::UnloadImage(img_);

  img_ = img;
  img_owned_ = false;
}

void Sprite::ReplaySprite()
{
  eclipsed_time_ = 0;
}

Image *Sprite::image() { return img_; }

void Sprite::Load(const Metric& metric)
{
  BaseObject::Load(metric);

  if (metric.exist("blend"))
    blending_ = metric.get<int>("blend");

  if (metric.exist("sprite"))
    LoadFromLR2SRC(metric.get<std::string>("sprite"));
  else if (metric.exist("lr2src"))
    LoadFromLR2SRC(metric.get<std::string>("lr2src"));
}

void Sprite::doRender()
{
  // If hide, then not draw
  if (!img_ || !IsVisible())
    return;

  Graphic::SetTextureId(img_->get_texture_ID());
  Graphic::SetBlendMode(blending_);

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

void Sprite::LoadFromLR2SRC(const std::string &cmd)
{
  /* imgname,sx,sy,sw,sh,divx,divy,cycle,timer */
  CommandArgs args(cmd, 12);

  SetImageByPath(args.Get<std::string>(0));

  if (!img_)
    return;

  source_x = args.Get<int>(1);
  source_y = args.Get<int>(2);
  source_width = args.Get<int>(3);
  source_height = args.Get<int>(4);

  sx_ = source_x / (float)img_->get_width();
  sy_ = source_y / (float)img_->get_height();
  if (source_width < 0) sw_ = 1.0f;
  else sw_ = source_width / (float)img_->get_width();
  if (source_height < 0) sh_ = 1.0f;
  else sh_ = source_height / (float)img_->get_height();

  int divx = args.Get<int>(5);
  int divy = args.Get<int>(6);

  divx_ = divx > 0 ? divx : 1;
  divy_ = divy > 0 ? divy : 1;
  cnt_ = divx_ * divy_;
  interval_ = args.Get<int>(7);

  SetSize(source_width / divx_, source_height / divy_);

  if (args.size() > 8)
  {
    // Register event : Sprite restarting
    AddCommand("LR" + args.Get<std::string>(8), "replay");
  }
}

const CommandFnMap& Sprite::GetCommandFnMap()
{
  static CommandFnMap cmdfnmap_;
  if (cmdfnmap_.empty())
  {
    cmdfnmap_ = BaseObject::GetCommandFnMap();
    cmdfnmap_["replay"] = [](void *o, CommandArgs& args) {
      static_cast<Sprite*>(o)->ReplaySprite();
    };
  }

  return cmdfnmap_;
}

}