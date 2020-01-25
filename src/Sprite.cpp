#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Script.h"
#include "Util.h"
#include <iostream>
#include <algorithm>

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), img_owned_(true),
    idx_(0), eclipsed_time_(0),
    tex_attribute_(0)
{
  ani_info_.divx = ani_info_.divy = ani_info_.cnt = 1;
  ani_info_.duration = 0;
  texcoord_.x = texcoord_.y = 0.f;
  texcoord_.w = texcoord_.h = 1.f;
  memset(&imgcoord_, 0, sizeof(imgcoord_));
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
  texcoord_.x = texcoord_.y = 0.f;
  texcoord_.w = texcoord_.h = 1.f;
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

void Sprite::SetImageCoordRect(const Rect &r)
{
  imgcoord_ = r;
  texcoord_.x = imgcoord_.x / (float)img_->get_width();
  texcoord_.y = imgcoord_.y / (float)img_->get_height();
  if (imgcoord_.w < 0) imgcoord_.w = 1.0f;
  else texcoord_.w = imgcoord_.w / (float)img_->get_width();
  if (imgcoord_.h < 0) imgcoord_.h = 1.0f;
  else texcoord_.h = imgcoord_.h / (float)img_->get_height();
}

void Sprite::SetTextureCoordRect(const RectF &r)
{
  texcoord_ = r;
  imgcoord_.x = texcoord_.x * (float)img_->get_width();
  imgcoord_.y = texcoord_.y * (float)img_->get_height();
  imgcoord_.w = texcoord_.w * (float)img_->get_width();
  imgcoord_.h = texcoord_.h * (float)img_->get_height();
}

const Rect& Sprite::GetImageCoordRect() { return imgcoord_; }

const RectF& Sprite::GetTextureCoordRect() { return texcoord_; }

const SpriteAnimationInfo&
Sprite::GetSpriteAnimationInfo() { return ani_info_; }

Image *Sprite::image() { return img_; }

void Sprite::Load(const Metric& metric)
{
  BaseObject::Load(metric);

  if (metric.exist("blend"))
    blending_ = metric.get<int>("blend");
}

void Sprite::GetVertexInfoOfFrame(VertexInfo* vi, size_t frame)
{
  float sw = texcoord_.w / ani_info_.divx;
  float sh = texcoord_.h / ani_info_.divy;
  float sx = texcoord_.x + sw * (frame % ani_info_.divx);
  float sy = texcoord_.y + sh * (frame / ani_info_.divx % ani_info_.divy);

  vi[0].sx = sx;
  vi[0].sy = sy;
  vi[1].sx = sx + sw;
  vi[1].sy = sy;
  vi[2].sx = sx + sw;
  vi[2].sy = sy + sh;
  vi[3].sx = sx;
  vi[3].sy = sy + sh;
}

void Sprite::SetNumber(int number)
{
  if (ani_info_.duration <= 0)
  {
    number = std::min(std::max(number, 0), ani_info_.cnt - 1);
    idx_ = number;
  }
}

void Sprite::Refresh()
{
  auto id = GetResourceId();
  if (id >= 0)
    SetNumber(Script::getInstance().GetNumber(id));
}

void Sprite::doRender()
{
  // If not loaded or hide, then not draw
  if (!img_ || !img_->is_loaded() || !IsVisible())
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

  GetVertexInfoOfFrame(vi_, idx_);

  Graphic::RenderQuad();
}

// milisecond
void Sprite::doUpdate(float delta)
{
  // update sprite info
  if (ani_info_.duration > 0)
  {
    eclipsed_time_ += delta;
    idx_ = eclipsed_time_ * ani_info_.divx * ani_info_.divy / ani_info_.duration % ani_info_.cnt;
    eclipsed_time_ = eclipsed_time_ % ani_info_.duration;
  }
}

void Sprite::LoadFromLR2SRC(const std::string &cmd)
{
  /* imgname,sx,sy,sw,sh,divx,divy,cycle,timer */
  CommandArgs args(cmd, 12);

  SetImageByPath(args.Get<std::string>(0));

  if (!img_ || !img_->is_loaded())
    return;

  Rect r;
  r.x = args.Get<int>(1);
  r.y = args.Get<int>(2);
  r.w = args.Get<int>(3);
  r.h = args.Get<int>(4);
  SetImageCoordRect(r);

  int divx = args.Get<int>(5);
  int divy = args.Get<int>(6);

  ani_info_.divx = divx > 0 ? divx : 1;
  ani_info_.divy = divy > 0 ? divy : 1;
  ani_info_.cnt = ani_info_.divx *  ani_info_.divy;
  ani_info_.duration = args.Get<int>(7);

  // set default sprite size
  SetSize(imgcoord_.w / ani_info_.divx, imgcoord_.h / ani_info_.divy);

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