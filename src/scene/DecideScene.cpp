#include "DecideScene.h"
#include "Song.h"

namespace rhythmus
{

DecideScene::DecideScene()
{
  set_name("DecideScene");
  // default action: go to GamePlay scene
  next_scene_mode_ = GameSceneMode::kGameSceneModePlay;
}

void DecideScene::LoadScene()
{
  Scene::LoadScene();
}

bool DecideScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyUp() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    // in case of song preload
    SongResource::getInstance().CancelLoad();
    next_scene_mode_ = GameSceneMode::kGameSceneModeSelect;
    TriggerFadeOut();
    return false;
  }

  if (!is_input_available())
    return true;

  return true;
}

}