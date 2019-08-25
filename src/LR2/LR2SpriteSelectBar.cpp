#include "LR2SpriteSelectBar.h"
#include "SceneManager.h"
#include "scene/SelectScene.h"

namespace rhythmus
{

/* internal usage. */
enum RenderType
{
  kRenderNone,
  kRenderBar,
};

LR2SpriteSelectBar::LR2SpriteSelectBar()
  : lr2_img_arrays_(nullptr)
{
  memset(bar_src_by_types_, 0, sizeof(bar_src_by_types_));
  set_name("BAR_BODY");
}

LR2SpriteSRC& LR2SpriteSelectBar::get_bar_src(int type_no)
{
  return bar_src_by_types_[type_no];
}

LR2SprInfo& LR2SpriteSelectBar::get_bar_sprinfo(int idx, int on_or_off)
{
  ASSERT(idx >= 0 && idx < 100 && on_or_off >= 0 && on_or_off < 2);
  return bar_sprinfo_[idx + on_or_off * 100];
}

void LR2SpriteSelectBar::set_lr2_img_arrays(std::vector<ImageAuto>& lr2_img_arrays)
{
  lr2_img_arrays_ = &lr2_img_arrays;
}

void LR2SpriteSelectBar::SetSpriteFromLR2Data()
{
  SelectScene* curr_scene =
    (SelectScene*)SceneManager::getInstance().get_current_scene();
  ASSERT(curr_scene);

  // Set SRC image to SelectScene
  // TODO: may need to set proper alias ...
  if (lr2_img_arrays_)
  {
    for (int i = 0; i < 10; ++i) if (bar_src_by_types_[i].imgidx > 0)
      curr_scene->SetSelectBarImage(i, lr2_img_arrays_->at(bar_src_by_types_[i].imgidx));
  }

  // TODO: Send position info to SelectScene
}

void LR2SpriteSelectBar::Render()
{
  SelectScene* curr_scene =
    (SelectScene*)SceneManager::getInstance().get_current_scene();
  ASSERT(curr_scene);

  curr_scene->DrawSelectBar();
}

}