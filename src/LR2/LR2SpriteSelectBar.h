#pragma once

#include "LR2SpriteDummy.h"

namespace rhythmus
{

/**
 * @brief
 * This class is for BAR_BODY
 */
class LR2SpriteSelectBar : public LR2SpriteDummy
{
public:
  LR2SpriteSelectBar();

  LR2SpriteSRC& get_bar_src(int type_no);
  LR2SprInfo& get_bar_sprinfo(int idx, int on_or_off);
  void set_lr2_img_arrays(std::vector<ImageAuto>& lr2_img_arrays);

  /* This method actually uploads sprite information to theme param (or Scene state)
   * So, scene must be set(allocated) before this method is called. */
  virtual void SetSpriteFromLR2Data();

  /* As this object doesn't have it's own rendering context,
   * It delegates rendering to pre-set sprite. */
  virtual void Render();

private:
  std::vector<ImageAuto>* lr2_img_arrays_;
  LR2SpriteSRC bar_src_by_types_[10];
  LR2SprInfo bar_sprinfo_[100 * 2]; /* on, off */
};

}