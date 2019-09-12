#include "ResultScene.h"

namespace rhythmus
{

ResultScene::ResultScene()
{
  set_name("ResultScene");
}

void ResultScene::LoadScene()
{
}

void ResultScene::StartScene()
{
  Game::getInstance().SetNextGameMode(GameMode::kGameModeSelect);
  Game::getInstance().ChangeGameMode();
}

void ResultScene::CloseScene()
{
}

bool ResultScene::ProcessEvent(const EventMessage& e)
{
  return true;
}

}