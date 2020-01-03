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

  if (!SongPlayer::getInstance().Load())
  {
    // exit scene instantly if no chart to play
    CloseScene(false);
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
      // trigger song ready event
      EventManager::SendEvent("PlayReady");
    });
    task->wait_for(theme_play_param_.load_wait_time);
    task->wait_cond([this] {
      return SongPlayer::getInstance().is_loaded() >= 2;
    });
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song ready ~ start
  {
    SceneTask *task = new SceneTask("songplaytask", [this] {
      // trigger song play event
      EventManager::SendEvent("PlayStart");
      SongPlayer::getInstance().Play();
      this->play_status_ = 1;
    });
    task->wait_for(theme_play_param_.ready_time);
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song finished
  {
    SceneTask *task = new SceneTask("songfinishedtask", [this] {
      //this->CloseScene(true);
      //this->play_status_ = 3;
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
#if 0
  /**
   * If we need to do early-playrecord saving, then enable this logic.
   * But SongPlayer::Stop() will automatically do that.
   * So, necessary?
   */
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
#endif

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
      SongPlayer::getInstance().Stop();
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
    SongPlayer::getInstance().Update(delta);
  }
}

}