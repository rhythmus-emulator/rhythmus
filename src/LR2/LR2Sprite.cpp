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

void LR2Sprite::SetSprite()
{
  // TODO
  //ani.SetAnimatedSource(sx, sy, sw, sh, divx, divy, timer, op1, op2, op3);
}

}