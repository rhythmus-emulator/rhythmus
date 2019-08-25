#include "LR2SpriteDummy.h"
#include "SceneManager.h"

namespace rhythmus
{

/* internal usage. */
enum RenderType
{
  kRenderNone,
  kRenderBar,
};

LR2SpriteDummy::LR2SpriteDummy()
{
  render_type_ = RenderType::kRenderNone;
}

void LR2SpriteDummy::set_name(const std::string& name)
{
  sprite_name_ = name;
}

void LR2SpriteDummy::SetSpriteFromLR2Data()
{
  Scene* curr_scene = SceneManager::getInstance().get_current_scene();
  ASSERT(curr_scene);

  if (curr_scene->GetSceneName() == "SelectScene")
  {
    if (get_name() == "BAR_BODY")
    {
      //curr_scene->SetBarSpriteSource(type, desc ...);
    }
    else if (get_name() == "BAR_BODY_OFF")
    {
      //curr_scene->SetBarSpriteTween(type, 0, desc ...);
    }
    else if (get_name() == "BAR_BODY_ON")
    {
      //curr_scene->SetBarSpriteTween(type, 1 - active, desc ...);
    }
    else if (get_name() == "BAR_TITLE")
    {
      //curr_scene->SetBarSpriteTextTween(type, 0 or 1, desc ...);
    }
    else if (get_name() == "BAR_LEVEL")
    {
      // TODO ...
    }
  }
}

void LR2SpriteDummy::Render()
{
  switch (render_type_)
  {
  case RenderType::kRenderBar:
    break;
  }
}

}