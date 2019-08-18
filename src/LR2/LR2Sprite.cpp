#include "LR2Sprite.h"

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

  ani_.SetAnimatedSource(sx, sy, sw, sh, divx, divy, 0);

  // Set DST
  // TODO
  //ani.SetAnimatedSource(sx, sy, sw, sh, divx, divy, timer, op1, op2, op3);

  ani_.SetPosition(get_cur_dst().x, get_cur_dst().y);
  ani_.SetSize(get_cur_dst().w, get_cur_dst().h);
}

}