#include "PlayScene.h"

namespace rhythmus
{

void PlayScene::LoadScene()
{
}

void PlayScene::StartScene()
{
  Game::getInstance().SetNextGameMode(GameMode::kGameModeResult);
  Game::getInstance().ChangeGameMode();
}

void PlayScene::CloseScene()
{
}

bool PlayScene::ProcessEvent(const EventMessage& e)
{
  return true;
}


const std::string PlayScene::GetSceneName() const
{
  return "PlayScene";
}

}