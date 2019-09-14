#include "ResultScene.h"

namespace rhythmus
{

ResultScene::ResultScene()
{
  set_name("ResultScene");
}

void ResultScene::LoadScene()
{
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "ResultScene", "../themes/WMIX_HD/result/WMIX_RESULT.lr2skin"
  );

  Scene::LoadScene();
}

void ResultScene::StartScene()
{
  Game::getInstance().SetNextGameMode(GameMode::kGameModeSelect);
}

void ResultScene::CloseScene()
{
  Scene::CloseScene();
}

bool ResultScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsInput() && !IsEventValidTime(e))
    return true;

  if (e.IsKeyDown())
  {
    CloseScene();
  }

  return true;
}

}