#include "LR2Sprite.h"
#include "Timer.h"
#include "LR2Flag.h"

namespace rhythmus
{

LR2Sprite::LR2Sprite()
{
  op_[0] = 0;
  op_[1] = 1;
  op_[2] = 2;
}

LR2SpriteSRC& LR2Sprite::get_src() { return src_; }

void LR2Sprite::new_dst() { dst_.emplace_back(LR2SpriteDST()); }

LR2SpriteDST& LR2Sprite::get_cur_dst()
{
  if (dst_.empty()) new_dst();
  return dst_.back();
}

void LR2Sprite::set_op(int op1, int op2, int op3)
{
  op_[0] = op1;
  op_[1] = op2;
  op_[2] = op3;
}

void LR2Sprite::set_timer(int timer_id)
{
  timer_id_ = timer_id;
}

void LR2Sprite::SetSpriteFromLR2Data()
{
  if (!img_ || !img_->is_loaded()) return;
  SetSpriteSRC();
  SetSpriteDST();
}

void LR2Sprite::SetSpriteSRC()
{
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
}

void LR2Sprite::SetSpriteDST()
{
  if (!dst_.empty())
  {
    // Before set DST, pre-calculate delta time
    int deltatime_calc[1024];
    ASSERT(dst_.size() < 1024);
    if (dst_[0].time > 0)
      ani_.AddTweenHideDuration(dst_[0].time);
    int i = 0;
    for (; i < dst_.size() - 1; ++i)
    {
      deltatime_calc[i] = dst_[i + 1].time - dst_[i].time;
    }
    deltatime_calc[i] = 0;

    // Set DST
    i = 0;
    for (const auto& dst : dst_)
    {
      ani_.AddTween(dst.x, dst.y, dst.w, dst.h,
        dst.r / 255.0f, dst.g / 255.0f, dst.b / 255.0f, dst.a / 255.0f,
        deltatime_calc[i], false);
      // TODO: set center
      // TODO: add AnimationFixed for loop process
      i++;
    }

    // Set Loop time
    ani_.SetTweenLoopTime(dst_.front().loop);

    // Set OP / Timer
    set_op(dst_.front().op1, dst_.front().op2, dst_.front().op3);
    set_timer(dst_.front().timer);
  }

  /* Only if you want to see last animation motion */
#if 0
  ani_.SetPosition(get_cur_dst().x, get_cur_dst().y);
  ani_.SetSize(get_cur_dst().w, get_cur_dst().h);
#endif
}

void LR2Sprite::Update()
{
  // TODO: reset animation if specified timer is reactivated.

  Sprite::Update();

  // hide sprite if currently showing & op(cond) is false
  if (ani_.IsDisplay() &&
      !(LR2Flag::GetFlag(op_[0]) && LR2Flag::GetFlag(op_[1]) &&
        LR2Flag::GetFlag(op_[2]) && LR2Flag::IsTimerActive(timer_id_))
     )
    ani_.Hide();
}

}