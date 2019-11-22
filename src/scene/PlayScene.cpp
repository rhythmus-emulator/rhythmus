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

  // TODO: check for bootmode
  next_scene_ = "ResultScene";
  prev_scene_ = "SelectScene";
}

void PlayScene::LoadScene()
{
  Scene::LoadScene();

  Metric *metric = Setting::GetThemeMetricList().get_metric(get_name());
  ASSERT(metric);
  theme_play_param_.load_wait_time = metric->get<int>("LoadingDelay");
  theme_play_param_.ready_time = metric->get<int>("ReadyDelay");

  // TODO
  size_t lanecount = 7;
  for (size_t i = 0; i < lanecount; ++i)
  {
    AddChild(&notefield_[i]);
    notefield_[i].set_player_id(0);
    notefield_[i].set_track_idx(i);

    // TODO: create new metric for notefield?
    // Lane1OnLoad --> OnLoad
    notefield_[i].Load(*metric);
  }

  // TODO: enqueue event: song loading
  // TODO: If course, enqueue charts to player.

  // attempt to call song load (if not loaded)
  // exit scene if no chart remain to play
  if (SongResource::getInstance().is_loaded() == 0)
  {
    std::string song_path;
    if (!Game::getInstance().pop_song(song_path))
    {
      CloseScene(false);
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
    EventManager::SendEvent("SongSelectChanged");
  }

  // send loading event
  EventManager::SendEvent("PlayLoading");

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
      EventManager::SendEvent("PlayReady");
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
      EventManager::SendEvent("PlayStart");

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
      this->CloseScene(true);
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

void PlayScene::CloseScene(bool next)
{
  if (next)
  {
    // Save record & replay for all players
    FOR_EACH_PLAYER(p, i)
    {
      if (p->GetPlayContext())
        p->GetPlayContext()->SavePlay();
    }
    END_EACH_PLAYER()
  }

  Scene::CloseScene(next);
}

void PlayScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyDown && e.KeyCode() == GLFW_KEY_ESCAPE)
  {
    FOR_EACH_PLAYER(p, i)
    {
      if (p->GetPlayContext())
        p->GetPlayContext()->StopPlay();
    }
    END_EACH_PLAYER()
    SongResource::getInstance().CancelLoad();
    FadeOutScene(false);
    return;
  }

  if (!is_input_available())
    return;

  FOR_EACH_PLAYER(p, i)
  {
    if (p->GetPlayContext())
      p->GetPlayContext()->ProcessInputEvent(e);
  }
  END_EACH_PLAYER()
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

}