#include "DecideScene.h"

namespace rhythmus
{

DecideScene::DecideScene()
{
  set_name("DecideScene");
}

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

}