#pragma once

#include "Sprite.h"
#include <vector>

namespace rhythmus
{

struct LR2SpriteSRC
{
  int imgidx,
    sx, sy, sw, sh,
    divx, divy, cycle, timer,
    op1, op2, op3;
};

struct LR2SpriteDST
{
  int time, x, y, w, h, acc_type, a, r, g, b,
    blend, filter, angle, center, loop, timer,
    op1, op2, op3;
};

/* @brief LR2 sprite info */
class LR2SprInfo
{
public:
  LR2SprInfo();

  LR2SpriteSRC& get_src();
  void new_dst();
  LR2SpriteDST& get_cur_dst();

  void set_op(int op1 = 0, int op2 = 0, int op3 = 0);
  void set_timer(int timer_id = 0);
  void set_src_size(int width, int height);

  /* @brief Set sprite information in sprite form */
  void SetSpriteFromLR2Data(SpriteAnimation& ani_);
  void SetSpriteSRC(SpriteAnimation& ani_);
  void SetSpriteDST(SpriteAnimation& ani_);

  bool IsSpriteShow();
private:
  LR2SpriteSRC src_;
  std::vector<LR2SpriteDST> dst_;
  int op_[3];
  int timer_id_;
  int width_, height_;
};

class LR2Sprite : public Sprite
{
public:
  LR2Sprite();

  LR2SprInfo& get_sprinfo();
  void SetSpriteFromLR2Data();

  /* @brief Update before rendering */
  virtual void Update();

private:
  LR2SprInfo spr_info_;
};

}