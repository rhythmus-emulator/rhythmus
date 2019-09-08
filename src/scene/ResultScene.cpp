#include "ResultScene.h"

namespace rhythmus
{

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


const std::string ResultScene::GetSceneName() const
{
  return "ResultScene";
}

}