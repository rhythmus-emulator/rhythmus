#include "PlayScene.h"

namespace rhythmus
{

PlayScene::PlayScene()
{
  set_name("PlayScene");
}

void PlayScene::LoadScene()
{
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "PlayScene", "../themes/WMIX_HD/play/HDPLAY_W.lr2skin"
  );

  Scene::LoadScene();
}

void PlayScene::StartScene()
{
  // next scene: result
  Game::getInstance().SetNextGameMode(GameMode::kGameModeResult);
  Scene::StartScene();
  //Game::getInstance().ChangeGameMode();
}

void PlayScene::CloseScene()
{
  Scene::CloseScene();
}

bool PlayScene::ProcessEvent(const EventMessage& e)
{
  return true;
}

}