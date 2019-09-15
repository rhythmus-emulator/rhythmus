#include "PlayScene.h"
#include "Song.h"

namespace rhythmus
{

PlayScene::PlayScene()
  : play_status_(0)
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

  // COMMENT:
  // Song loading is might be already started from SelectScene.
  // But, if it's not loaded (by some reason),
  // or if it loaded different song, then we need to reload it.
  // - TODO -
}

void PlayScene::CloseScene()
{
  Scene::CloseScene();
}

bool PlayScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsInput() && !IsEventValidTime(e))
    return true;

  if (e.IsKeyDown())
  {
    if (e.GetKeycode() == GLFW_KEY_ESCAPE)
      CloseScene();
  }

  return true;
}

void PlayScene::doUpdate(float delta)
{
  switch (play_status_)
  {
  case 0:
    if (SongPlayable::getInstance().IsLoaded())
    {
      // TODO: need to upload bitmap here

      // TODO: Tick game timer manually here,
      // as uploading bitmap may cost much time ...
      SongPlayable::getInstance().Play();
      play_status_ = 1;
    }
    break;
  case 3:
    break;
  }
}

}