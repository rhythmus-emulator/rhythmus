#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Script.h"
#include "Util.h"
#include "common.h"
#include "config.h"

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), time_(0), frame_(0), res_id_(nullptr),
    blending_(0), use_texture_coord_(true), tex_attribute_(0)
{
  sprani_.divx = sprani_.divy = sprani_.cnt = 1;
  sprani_.duration = 0;
  texcoord_ = Rect{ 0.0f, 0.0f, 1.0f, 1.0f };
}

Sprite::~Sprite()
{
  if (img_)
    IMAGEMAN->Unload(img_);
}

void Sprite::Load(const MetricGroup& metric)
{
  BaseObject::Load(metric);

  metric.get_safe("blend", blending_);

#if USE_LR2_FEATURE == 1
  if (metric.exist("lr2src"))
  {
    /* (null),imgname,sx,sy,sw,sh,divx,divy,cycle,timer */
    std::string lr2src;
    CommandArgs args(lr2src, 10);

    SetImage(args.Get<std::string>(0));

    Vector4 r{
      args.Get<int>(1), args.Get<int>(2), args.Get<int>(3), args.Get<int>(4)
    };
    // Use whole image if width/height is zero.
    if (r.z <= 0 || r.w <= 0)
      SetTextureCoord(Vector4{ 0.0f, 0.0f, 1.0f, 1.0f });
    else
      SetImageCoord(Vector4{r.x, r.y, r.x+r.z, r.y+r.w});

    int divx = args.Get<int>(5);
    int divy = args.Get<int>(6);

    sprani_.divx = divx > 0 ? divx : 1;
    sprani_.divy = divy > 0 ? divy : 1;
    sprani_.cnt = sprani_.divx *  sprani_.divy;
    sprani_.duration = args.Get<int>(7);
  }
  // TODO: load blending from lr2dst property
  // TODO: load resource id
#endif
}

void Sprite::SetImage(const std::string &path)
{
  if (img_)
  {
    IMAGEMAN->Unload(img_);
    img_ = nullptr;
  }

  img_ = IMAGEMAN->Load(path);
  if (!img_)
    return;

  /* default image texture coord set */
  use_texture_coord_ = true;
  texcoord_ = Rect{ 0.0f, 0.0f, 1.0f, 1.0f };
}

Image *Sprite::image() { return img_; }

void Sprite::SetBlending(int blend)
{
  blending_ = blend;
}

void Sprite::SetImageCoord(const Rect &r)
{
  texcoord_ = r;
  use_texture_coord_ = false;
}

void Sprite::SetTextureCoord(const Rect &r)
{
  texcoord_ = r;
  use_texture_coord_ = true;
}

void Sprite::Replay()
{
  time_ = 0;
  frame_ = 0;
}

const SpriteAnimationInfo&
Sprite::GetSpriteAnimationInfo() { return sprani_; }

void Sprite::SetNumber(int number)
{
  if (sprani_.duration <= 0)
  {
    number = std::min(std::max(number, 0), sprani_.cnt - 1);
    frame_ = number;
  }
}

void Sprite::Refresh()
{
  if (res_id_)
    SetNumber(*res_id_);
}

// milisecond
void Sprite::doUpdate(float delta)
{
  // update sprite frame
  if (sprani_.duration > 0)
  {
    time_ += delta;
    frame_ = (int)(time_ * sprani_.divx * sprani_.divy / sprani_.duration)
      % sprani_.cnt;
    time_ = fmod(time_, (float)sprani_.duration);
  }
}

void Sprite::doRender()
{
  VertexInfo vi[4];
  Vector4 texcrop;
  Point imgsize;

  // If not loaded or hide, then not draw
  if (!img_ || !img_->is_loaded() || !IsVisible())
    return;

  // calculate texture crop area
  imgsize = Point{ (float)img_->get_width(), (float)img_->get_height() };
  if (use_texture_coord_)
    texcrop = texcoord_;
  else
  {
    texcrop.x = texcoord_.x / imgsize.x;
    texcrop.y = texcoord_.y / imgsize.y;
    texcrop.z = texcoord_.z / imgsize.x;
    texcrop.w = texcoord_.w / imgsize.y;
  }

  // texture animation
  if (sprani_.divx > 1 || sprani_.divy > 1)
  {
    float w = GetWidth(texcrop) / sprani_.divx;
    float h = GetHeight(texcrop) / sprani_.divy;
    int ix = frame_ % sprani_.divx;
    int iy = frame_ / sprani_.divx % sprani_.divy;
    texcrop.x += w * ix;
    texcrop.y += h * ix;
    texcrop.z = texcrop.x + w;
    texcrop.w = texcrop.y + h;
  }

  BaseObject::FillVertexInfo(vi);
  vi[0].t = Point{ texcrop.x, texcrop.y };
  vi[1].t = Point{ texcrop.z, texcrop.y };
  vi[2].t = Point{ texcrop.z, texcrop.w };
  vi[3].t = Point{ texcrop.x, texcrop.w };

  GRAPHIC->SetTexture(0, img_->get_texture_ID());
  GRAPHIC->SetBlendMode(blending_);
  GRAPHIC->DrawQuad(vi);

#if 0
  // for predefined src width / height (-1 means use whole texture)
  if (ti.sw == -1) sx1 = 0.0, sx2 = 1.0;
  if (ti.sh == -1) sy1 = 0.0, sy2 = 1.0;
#endif
}

}