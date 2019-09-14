#include "DecideScene.h"

namespace rhythmus
{

DecideScene::DecideScene()
{
  set_name("DecideScene");
}

void DecideScene::LoadScene()
{
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "DecideScene", "../themes/WMIX_HD/decide/decide.lr2skin"
  );

  Scene::LoadScene();
}

void DecideScene::StartScene()
{
  // default action: go to GamePlay scene
  Game::getInstance().SetNextGameMode(GameMode::kGameModePlay);
  Scene::StartScene();
}

void DecideScene::CloseScene()
{
  Scene::CloseScene();
}

bool DecideScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsInput() && !IsEventValidTime(e))
    return true;

  if (e.IsKeyUp() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    CloseScene();
  }

  return true;
}

}