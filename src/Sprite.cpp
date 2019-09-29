#include "Sprite.h"
#include "Game.h"
#include "SceneManager.h" // to use scene timer
#include "Util.h"
#include <iostream>
#include <algorithm>

namespace rhythmus
{

Sprite::Sprite()
  : divx_(1), divy_(1), cnt_(1), interval_(0), blending_(1),
    idx_(0), eclipsed_time_(0),
    sx_(0), sy_(0), sw_(1.0f), sh_(1.0f), tex_attribute_(0)
{
}

Sprite::~Sprite()
{
}

void Sprite::SetImage(ImageAuto img)
{
  img_ = img;
}

void Sprite::SetImageByName(const std::string& name)
{
  auto img = SceneManager::getInstance().get_current_scene()->GetImageByName(name);
  SetImage(img);
}

void Sprite::SetBlend(int blend_mode)
{
  blending_ = blend_mode;
}

void Sprite::LoadSprite(const Sprite& spr)
{
  SetImage(spr.img_);
  divx_ = spr.divx_;
  divy_ = spr.divy_;
  cnt_ = spr.cnt_;
  interval_ = spr.interval_;
  idx_ = spr.idx_;
  eclipsed_time_ = spr.eclipsed_time_;
  sx_ = spr.sx_;
  sy_ = spr.sy_;
  sw_ = spr.sw_;
  sh_ = spr.sh_;
  tex_attribute_ = spr.tex_attribute_;
}

void Sprite::LoadProperty(const std::string& prop_name, const std::string& value)
{
  /* below is for LR2 type commands */
  if (strnicmp(prop_name.c_str(), "#SRC_", 5) == 0)
  {
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 13);
    int attr = atoi(params[0].c_str());
    int imgidx = atoi(params[1].c_str());
    int sx = atoi(params[2].c_str());
    int sy = atoi(params[3].c_str());
    int sw = atoi(params[4].c_str());
    int sh = atoi(params[5].c_str());
    int divx = atoi(params[6].c_str());
    int divy = atoi(params[7].c_str());
    int cycle = atoi(params[8].c_str());    /* total loop time */
    int timer = atoi(params[9].c_str());    /* timer id in LR2 form */
#if 0
    int op1 = atoi_op(params[10].c_str());
    int op2 = atoi_op(params[11].c_str());
    int op3 = atoi_op(params[12].c_str());
#endif

    SetImageByName(params[1]);

    if (img_)
    {
      sx_ = sx / (float)img_->get_width();
      sy_ = sy / (float)img_->get_height();
      if (sw < 0) sw_ = 1.0f;
      else sw_ = sw / (float)img_->get_width();
      if (sh < 0) sh_ = 1.0f;
      else sh_ = sh / (float)img_->get_height();
      divx_ = divx > 0 ? divx : 1;
      divy_ = divy > 0 ? divy : 1;
      cnt_ = divx_ * divy_;
      interval_ = cycle;
    }

    // this attribute will be processed in LR2Objects
    SetAttribute("src_timer", params[9]);

    return;
  }

  BaseObject::LoadProperty(prop_name, value);
}

void Sprite::Load()
{
  if (IsAttribute("blend"))
    blending_ = GetAttribute<int>("blend");
}

void Sprite::doRender()
{
  // If hide, then not draw
  if (!IsVisible())
    return;

  if (img_)
  {
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
  }
  else return;

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

}