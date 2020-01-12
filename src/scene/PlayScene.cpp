#include "PlayScene.h"
#include "SongPlayer.h"
#include "Util.h"
#include "Player.h"

namespace rhythmus
{

PlayScene::PlayScene()
  : play_status_(0)
{
  set_name("PlayScene");

  next_scene_ = "ResultScene";
  prev_scene_ = "SelectScene";

  if (Game::getInstance().get_boot_mode() == GameBootMode::kBootPlay)
  {
    next_scene_ = "Exit";
    prev_scene_ = "Exit";
  }
}

void PlayScene::LoadScene()
{
  if (!SongPlayer::getInstance().Load())
  {
    // exit scene instantly if no chart to play
    CloseScene(false);
  }

  Metric *metric = Setting::GetThemeMetricList().get_metric(get_name());
  ASSERT(metric);
  theme_play_param_.load_wait_time = metric->get<int>("LoadingDelay");
  theme_play_param_.ready_time = metric->get<int>("ReadyDelay");

  // Create notefield object per player.
  FOR_EACH_PLAYER(p, i)
  {
    notefield_[i].set_name(format_string("NoteField%dP", i + 1));
    notefield_[i].set_player(i);
    AddChild(&notefield_[i]);
  }
  END_EACH_PLAYER()

  Scene::LoadScene();
}

void PlayScene::StartScene()
{
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
    SongPlayer::getInstance().Stop();
    FadeOutScene(false);
    return;
  }

  if (!is_input_available())
    return;

  SongPlayer::getInstance().ProcessInputEvent(e);
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