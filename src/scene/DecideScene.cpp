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
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "DecideScene", "../themes/WMIX_HD/decide/decide.lr2skin"
  );

  Scene::LoadScene();
}

bool DecideScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyUp() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    // in case of song preload
    SongPlayable::getInstance().CancelLoad();
    TriggerFadeOut();
  }

  if (!is_input_available())
    return true;

  return true;
}

}