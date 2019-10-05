#include "PlayScene.h"
#include "Song.h"
#include "Player.h"

namespace rhythmus
{

PlayScene::PlayScene()
  : play_status_(0)
{
  set_name("PlayScene");

  // next scene: result
  next_scene_mode_ = GameSceneMode::kGameSceneModeResult;
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
  Scene::StartScene();
  //Game::getInstance().ChangeGameMode();

  // Prepare each player to start recording
  {
    Player *p;
    int i;
    FOR_EACH_PLAYER(p, i)
      p->StartPlay();
  }

  // Song loading is might be already started from SelectScene.
  // But, if it's not loaded (by some reason),
  // or if it loaded different song, then we need to reload it.
  if (!SongPlayable::getInstance().IsLoading() &&
      !SongPlayable::getInstance().IsLoaded())
  {
    // TODO: reload song
  }
}

void PlayScene::CloseScene()
{
  // Save record & replay for all players
  {
    Player *p;
    int i;
    FOR_EACH_PLAYER(p, i)
      p->SavePlay();
  }

  Scene::CloseScene();
}

bool PlayScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    SongPlayable::getInstance().CancelLoad();
    TriggerFadeOut();
    return false;
  }

  if (!is_input_available())
    return true;

  return true;
}

void PlayScene::doUpdate(float delta)
{
  Scene::doUpdate(delta);

  switch (play_status_)
  {
  case 0:
    if (SongPlayable::getInstance().IsLoaded())
    {
      // trigger event
      EventManager::SendEvent(Events::kEventSongLoadFinished);

      // TODO: need to upload bitmap here

      // TODO: ready? timer

      // TODO: Tick game timer manually here,
      // as uploading bitmap may cost much time ...

      // Play song & trigger players to start
      {
        int i;
        Player *p;
        FOR_EACH_PLAYER(p, i)
          p->StartPlay();
      }
      SongPlayable::getInstance().Play();
      play_status_ = 1;
    }
    break;
  case 1:
    SongPlayable::getInstance().Update(delta);
    if (SongPlayable::getInstance().IsPlayFinished())
    {
      CloseScene();
      play_status_ = 3;
    }
    break;
  case 2:
  case 3:
    // don't do anything
    break;
  }

  // Update all players
  {
    int i;
    Player *p;
    FOR_EACH_PLAYER(p, i)
      p->Update(delta);
  }
}

}