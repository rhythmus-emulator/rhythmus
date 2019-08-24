#include "LR2SpriteDummy.h"
#include "SceneManager.h"

namespace rhythmus
{

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
    if (get_name() == "SRC_BAR_BODY")
    {
    }
  }
}

void LR2SpriteDummy::Render()
{
  // TODO
}

}