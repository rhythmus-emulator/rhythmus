#include "DecideScene.h"

namespace rhythmus
{

void DecideScene::LoadScene()
{
}

void DecideScene::StartScene()
{
  Game::getInstance().SetNextGameMode(GameMode::kGameModePlay);
  Game::getInstance().ChangeGameMode();
}

void DecideScene::CloseScene()
{
}

bool DecideScene::ProcessEvent(const EventMessage& e)
{
  return true;
}


const std::string DecideScene::GetSceneName() const
{
  return "DecideScene";
}

}