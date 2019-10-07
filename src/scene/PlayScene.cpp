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
}

void PlayScene::StartScene()
{
  // In case of Play-only boot mode:
  // trigger event force to trigger some related events.
  if (Game::getInstance().get_boot_mode() == GameBootMode::kBootPlay)
  {
    EventManager::SendEvent(Events::kEventSongSelectChanged);
  }

  // attempt to call song load (if not loaded)
  if (!SongPlayable::getInstance().IsLoading() &&
      !SongPlayable::getInstance().IsLoaded())
  {
    std::string path, song_path, chart_name;
    song_path = SongList::getInstance().get_current_song_info()->songpath;
    chart_name = SongList::getInstance().get_current_song_info()->chartpath;
    SongPlayable::getInstance().LoadAsync(song_path, chart_name);
  }

  // send loading event
  EventManager::SendEvent(Events::kEventPlayLoading);

  // enqueue event: song loading
  {
    SceneTask *task = new SceneTask("songreadytask", [this] {
      // need to upload bitmap here
      SongPlayable::getInstance().UploadBgaImages();

      // TODO: Tick game timer manually here,
      // as uploading bitmap may cost much time ...

      // trigger song ready event
      EventManager::SendEvent(Events::kEventPlayReady);
    });
    task->wait_for(theme_play_param_.load_wait_time);
    task->wait_cond([this] {
      return SongPlayable::getInstance().IsLoaded();
    });
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song ready ~ start
  {
    SceneTask *task = new SceneTask("songplaytask", [this] {
      // trigger song play event
      EventManager::SendEvent(Events::kEventPlayStart);

      // Play song & trigger players to start
      {
        int i;
        Player *p;
        FOR_EACH_PLAYER(p, i)
        {
          p->StartPlay();
        }
      }
      SongPlayable::getInstance().Play();
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
      return SongPlayable::getInstance().IsPlayFinished();
    });
    playscenetask_.Enqueue(task);
  }

  // do default StartScene() function
  Scene::StartScene();

  // Prepare each player to start recording
  {
    Player *p;
    int i;
    FOR_EACH_PLAYER(p, i)
      p->StartPlay();
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
  playscenetask_.Update(delta);

  if (play_status_ == 1)
  {
    SongPlayable::getInstance().Update(delta);
  }

  // Update all players
  {
    int i;
    Player *p;
    FOR_EACH_PLAYER(p, i)
      p->Update(delta);
  }
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