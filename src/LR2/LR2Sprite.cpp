#include "LR2Sprite.h"
#include "Timer.h"

namespace rhythmus
{

LR2SpriteSRC& LR2Sprite::get_src() { return src_; }

void LR2Sprite::new_dst() { dst_.emplace_back(LR2SpriteDST()); }

LR2SpriteDST& LR2Sprite::get_cur_dst()
{
  if (dst_.empty()) new_dst();
  return dst_.back();
}

void LR2Sprite::SetSpriteFromLR2Data()
{
  if (!img_ || !img_->is_loaded()) return;

  // Set SRC
  float sx, sy, sw, sh;
  sx = (float)src_.sx / img_->get_width();
  sy = (float)src_.sy / img_->get_height();
  if (src_.sw < 0)
    sw = 1.0f - sx;
  else
    sw = (float)src_.sw / img_->get_width();
  if (src_.sh < 0)
    sh = 1.0f - sy;
  else
    sh = (float)src_.sh / img_->get_height();

  int divx = src_.divx, divy = src_.divy;
  if (divx < 1) divx = 1;
  if (divy < 1) divy = 1;

  ani_.UseAnimatedTexture(true);
  ani_.SetAnimatedSource(sx, sy, sw, sh, divx, divy, 0, src_.cycle);

  // Set DST
  int cur_time = 0;
  for (const auto& dst : dst_)
  {
    ani_.AddTween(dst.x, dst.y, dst.w, dst.h,
      dst.r / 255.0f, dst.g / 255.0f, dst.b / 255.0f, dst.a / 255.0f,
      dst.time - cur_time, false);
    // TODO: set center
    // TODO: add AnimationFixed for loop process
    cur_time = dst.time;
  }
  
  /* Only if you want to see last animation motion */
#if 0
  ani_.SetPosition(get_cur_dst().x, get_cur_dst().y);
  ani_.SetSize(get_cur_dst().w, get_cur_dst().h);
#endif
}

void LR2Sprite::Update()
{
  // TODO: update animation if specified timer is activated.

  Sprite::Update();
}

}