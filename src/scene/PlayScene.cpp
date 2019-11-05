#include "PlayScene.h"
#include "Song.h"
#include "Util.h"
#include "Player.h"

namespace rhythmus
{

PlayScene::PlayScene()
  : play_status_(0)
{
  set_name("PlayScene");

  memset(&theme_play_param_, 0, sizeof(theme_play_param_));

  // next scene: result
  next_scene_mode_ = GameSceneMode::kGameSceneModeResult;
}

void PlayScene::LoadScene()
{
  Scene::LoadScene();

  // TODO: enqueue event: song loading
  // TODO: If course, enqueue charts to player.

  // attempt to call song load (if not loaded)
  // exit scene if no chart remain to play
  if (SongResource::getInstance().is_loaded() == 0)
  {
    std::string song_path;
    if (!Game::getInstance().pop_song(song_path))
    {
      CloseScene();
      return;
    }
    SongResource::getInstance().LoadSong(song_path);

    SongResource::getInstance().LoadResourcesAsync();
  }
}

void PlayScene::StartScene()
{
  // In case of Play-only boot mode:
  // trigger event force to trigger some related events.
  if (Game::getInstance().get_boot_mode() == GameBootMode::kBootPlay)
  {
    EventManager::SendEvent(Events::kEventSongSelectChanged);
  }

  // send loading event
  EventManager::SendEvent(Events::kEventPlayLoading);

  // enqueue event: after song resource loading
  {
    SceneTask *task = new SceneTask("songreadytask", [this] {
      // need to upload bitmap here
      SongResource::getInstance().UploadBitmaps();

      // fetch chart / resource for each player
      FOR_EACH_PLAYER(p, i)
      {
        p->LoadNextChart();
      }
      END_EACH_PLAYER()

      // trigger song ready event
      EventManager::SendEvent(Events::kEventPlayReady);
    });
    task->wait_for(theme_play_param_.load_wait_time);
    task->wait_cond([this] {
      return SongResource::getInstance().is_loaded() >= 2;
    });
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song ready ~ start
  {
    SceneTask *task = new SceneTask("songplaytask", [this] {
      // trigger song play event
      EventManager::SendEvent(Events::kEventPlayStart);

      // Play song & trigger players to start
      FOR_EACH_PLAYER(p, i)
      {
        p->GetPlayContext()->StartPlay();
      }
      END_EACH_PLAYER()
      this->play_status_ = 1;
    });
    task->wait_for(theme_play_param_.ready_time);
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song finished
  {
    SceneTask *task = new SceneTask("songfinishedtask", [this] {
      this->CloseScene();
      this->play_status_ = 3;
    });
    task->wait_cond([this] {
      return this->play_status_ == 1 && Player::IsAllPlayerFinished();
    });
    playscenetask_.Enqueue(task);
  }

  // do default StartScene() function
  Scene::StartScene();
}

void PlayScene::CloseScene()
{
  // Save record & replay for all players
  FOR_EACH_PLAYER(p, i)
  {
    if (p->GetPlayContext())
      p->GetPlayContext()->SavePlay();
  }
  END_EACH_PLAYER()

  Scene::CloseScene();
}

bool PlayScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    FOR_EACH_PLAYER(p, i)
    {
      if (p->GetPlayContext())
        p->GetPlayContext()->StopPlay();
    }
    END_EACH_PLAYER()
    SongResource::getInstance().CancelLoad();
    TriggerFadeOut();
    return false;
  }

  if (!is_input_available())
    return true;

  FOR_EACH_PLAYER(p, i)
  {
    if (p->GetPlayContext())
      p->GetPlayContext()->ProcessInputEvent(e);
  }
  END_EACH_PLAYER()

  return true;
}

void PlayScene::doUpdate(float delta)
{
  Scene::doUpdate(delta);
  playscenetask_.Update(delta);

  // Play BGM while playing.
  if (play_status_ == 1)
  {
    SongResource::getInstance().Update(delta);
  }

  // Update all players
  FOR_EACH_PLAYER(p, i)
  {
    if (p->GetPlayContext())
      p->GetPlayContext()->Update(delta);
  }
  END_EACH_PLAYER()
}

void PlayScene::LoadProperty(const std::string& prop_name, const std::string& value)
{
  if (prop_name == "#SCRATCHSIDE")
  {
    theme_play_param_.playside = atoi(value.c_str());
  }
  else if (prop_name == "#LOADSTART")
  {
    // ignore this parameter
    //theme_play_param_.loadend_wait_time += atoi(value.c_str());
  }
  else if (prop_name == "#LOADEND")
  {
    theme_play_param_.load_wait_time = atoi(value.c_str());
  }
  else if (prop_name == "#PLAYSTART")
  {
    theme_play_param_.ready_time = atoi(value.c_str());
  }
  else Scene::LoadProperty(prop_name, value);
}

}