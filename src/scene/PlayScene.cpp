#include "PlayScene.h"

namespace rhythmus
{

PlayScene::PlayScene()
{
  set_name("PlayScene");
}

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

}