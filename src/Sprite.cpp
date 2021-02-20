#include "Sprite.h"
#include "Game.h"
#include "ResourceManager.h"
#include "Script.h"
#include "Logger.h"
#include "KeyPool.h"
#include "Util.h"
#include "common.h"
#include "config.h"
#include <sstream>

namespace rhythmus
{

Sprite::Sprite()
  : img_(nullptr), time_(0), frame_(0), res_id_(nullptr),
    blending_(0), filtering_(0), use_texture_coord_(true), tex_attribute_(0)
{
  set_xy_as_center_ = true;
  sprani_.divx = sprani_.divy = sprani_.cnt = 1;
  sprani_.duration = 0;
  texcoord_ = Rect{ 0.0f, 0.0f, 1.0f, 1.0f };
}

Sprite::Sprite(const Sprite& spr) : BaseObject(spr),
  img_(nullptr), sprani_(spr.sprani_), time_(0), frame_(0), res_id_(spr.res_id_),
  blending_(spr.blending_), filtering_(spr.filtering_), texcoord_(spr.texcoord_),
  use_texture_coord_(spr.use_texture_coord_), tex_attribute_(spr.tex_attribute_)
{
  if (spr.img_)
    img_ = (Image*)spr.img_->clone();
}

Sprite::~Sprite()
{
  if (img_)
    IMAGEMAN->Unload(img_);
}

BaseObject *Sprite::clone()
{
  return new Sprite(*this);
}

void Sprite::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  m.get_safe("blend", blending_);

  if (m.exist("path"))
    SetImage(m.get_str("path"));
  else if (m.exist("src"))
    SetImage(m.get_str("src"));

  if (m.exist("crop"))
  {
    CommandArgs args(m.get_str("crop"), 4, true);
    SetImageCoord(Vector4{ args.Get<int>(0), args.Get<int>(1),
      args.Get<int>(2), args.Get<int>(3) });
  }
  else if (m.exist("croptex"))
  {
    CommandArgs args(m.get_str("croptex"), 4, true);
    SetTextureCoord(Vector4{ args.Get<int>(0), args.Get<int>(1),
      args.Get<int>(2), args.Get<int>(3) });
  }
}

void Sprite::OnReady()
{
  // TODO: use texture coord later
}

void Sprite::SetImage(const std::string &path)
{
  if (img_)
  {
    IMAGEMAN->Unload(img_);
    img_ = nullptr;
  }

  path_ = path;
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

void Sprite::SetFiltering(int filtering)
{
  filtering_ = filtering;
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

void Sprite::SetAnimatedTexture(int divx, int divy, int duration)
{
  sprani_.divx = divx > 0 ? divx : 1;
  sprani_.divy = divy > 0 ? divy : 1;
  sprani_.cnt = sprani_.divx *  sprani_.divy;
  sprani_.duration = duration;
}

void Sprite::ReplaySprite()
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

void Sprite::SetResourceId(const std::string &id)
{
  auto k = KEYPOOL->GetInt(id);
  res_id_ = &*k;
}

void Sprite::Refresh()
{
  if (res_id_)
    SetNumber(*res_id_);
}

void Sprite::SetDuration(int milisecond)
{
  sprani_.duration = milisecond;
}

// milisecond
void Sprite::doUpdate(double delta)
{
  // update sprite frame
  if (sprani_.duration > 0)
  {
    time_ += delta;
    frame_ = (int)(time_ * sprani_.divx * sprani_.divy / sprani_.duration)
      % sprani_.cnt;
    time_ = fmod(time_, (double)sprani_.duration);
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
    float w = rhythmus::GetWidth(texcrop) / sprani_.divx;
    float h = rhythmus::GetHeight(texcrop) / sprani_.divy;
    int ix = frame_ % sprani_.divx;
    int iy = frame_ / sprani_.divx % sprani_.divy;
    texcrop.x += w * ix;
    texcrop.y += h * iy;
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
  // TODO: SetFiltering in graphic.
  GRAPHIC->DrawQuad(vi);
}

const char* Sprite::type() const { return "sprite"; }

std::string Sprite::toString() const
{
  std::stringstream ss;
  if (img_)
  {
    ss << "file: " << img_->get_path() << "," << img_->get_width() << "," << img_->get_height() << std::endl;
  }
  else {
    ss << "file is empty." << std::endl;
  }
  return BaseObject::toString() + ss.str();
}

// ------------------------------------------------------------------ Loader/Helper

class LR2CSVSpriteHandlers
{
public:
  static bool src_image(Sprite *&o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    Vector4 r{
      ctx->get_int(3), ctx->get_int(4), ctx->get_int(5), ctx->get_int(6)
    };
    // Use whole image if width/height is zero.
    o->SetImage(format_string("image%s", ctx->get_str(2)));
    if (r.z < 0 || r.w < 0)
      o->SetTextureCoord(Vector4{ 0.0f, 0.0f, 1.0f, 1.0f });
    else
      o->SetImageCoord(Vector4{ r.x, r.y, r.x + r.z, r.y + r.w });
    o->SetAnimatedTexture(ctx->get_int(7), ctx->get_int(8), ctx->get_int(9));
    return true;
  }

  static bool dst_image(Sprite*& o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // XXX: check object type before execution to prevent user-fault ...
    // But it may cause side effect in case of inherited object, so be careful.
    if (loader->get_command_index() == 0) {
      o->SetBlending(ctx->get_int(12));
      o->SetFiltering(ctx->get_int(13));
    }
    return true;
  }

  LR2CSVSpriteHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_IMAGE", (LR2CSVHandlerFunc)&src_image);
    LR2CSVExecutor::AddTrigger("#SRC_IMAGE", "#SRC_BASE_");
    LR2CSVExecutor::AddHandler("#DST_IMAGE", (LR2CSVHandlerFunc)&dst_image);
    LR2CSVExecutor::AddTrigger("#DST_IMAGE", "#DST_BASE_");
  }
};

// register handlers
LR2CSVSpriteHandlers _LR2CSVSpriteHandlers;

}
