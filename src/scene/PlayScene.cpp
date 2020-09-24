#include "PlayScene.h"
#include "SongPlayer.h"
#include "Util.h"
#include "Player.h"
#include "Script.h"
#include "SceneManager.h"

namespace rhythmus
{

PlayScene::PlayScene()
  : play_status_(0)
{
  set_name("PlayScene");

  next_scene_ = "ResultScene";
  prev_scene_ = "SelectScene";

  if (GAME->get_boot_mode() == GameBootMode::kBootPlay)
  {
    next_scene_ = "Exit";
    prev_scene_ = "Exit";
  }
}

void PlayScene::Load(const MetricGroup& m)
{
  m.get_safe("LoadingDelay", theme_play_param_.load_wait_time);
  m.get_safe("ReadyDelay", theme_play_param_.ready_time);
}

void PlayScene::LoadScene()
{
  if (!SongPlayer::getInstance().LoadNext())
  {
    // exit scene instantly if no chart to play
    CloseScene(false);
  }

  Scene::LoadScene();

  // Create notefield object per player.
  FOR_EACH_PLAYER(p, i)
  {
    notefield_[i].set_name(format_string("NoteField%dP", i + 1));
    notefield_[i].set_player(i);
    AddChild(&notefield_[i]);
  }
  END_EACH_PLAYER()
}

void PlayScene::StartScene()
{
  // send loading event
  EVENTMAN->SendEvent("PlayLoading");

  // enqueue event: after song resource loading
  {
    SceneTask *task = new SceneTask("songreadytask", [this] {
      // trigger song ready event
      EVENTMAN->SendEvent("PlayReady");
    });
    task->wait_for(theme_play_param_.load_wait_time);
    task->wait_cond([this] {
      return SongPlayer::getInstance().is_loaded();
    });
    playscenetask_.Enqueue(task);
  }

  // enqueue event: song ready ~ start
  {
    SceneTask *task = new SceneTask("songplaytask", [this] {
      // trigger song play event
      EVENTMAN->SendEvent("PlayStart");
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
      return this->play_status_ == 1 && SongPlayer::getInstance().is_play_finished();
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
  if (e.type() == InputEvents::kOnKeyDown)
  {
    if (e.KeyCode() == RI_KEY_ESCAPE)
    {
      SongPlayer::getInstance().Stop();
      FadeOutScene(false);
      return;
    }
    else if (e.KeyCode() == RI_KEY_TAB)
    {
      GAME->AlertMessageBox("Game Pause", "Game Paused.");
    }
  }

  SongPlayer::getInstance().ProcessInputEvent(e);
}

void PlayScene::doUpdate(double delta)
{
  Scene::doUpdate(delta);
  playscenetask_.Update((float)delta);
}

void PlayScene::SetPlayer(int player) { theme_play_param_.playside = player; }
void PlayScene::SetMinimumLoadingTime(int time) { theme_play_param_.load_wait_time = time; }
void PlayScene::SetReadyTime(int time) { theme_play_param_.ready_time = time; }


class LR2CSVPlaySceneHandlers
{
public:
  static void loadstart(LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // ignored.
  }
  static void loadend(LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (PlayScene*)loader->get_object("playscene");
    if (!scene) return;
    scene->SetMinimumLoadingTime(ctx->get_int(1));
  }
  static void scratchside(LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (PlayScene*)loader->get_object("playscene");
    if (!scene) return;
    scene->SetPlayer(ctx->get_int(1));
  }
  LR2CSVPlaySceneHandlers()
  {
    LR2CSVExecutor::AddHandler("#LOADSTART", (LR2CSVCommandHandler)&loadstart);
    LR2CSVExecutor::AddHandler("#LOADEND", (LR2CSVCommandHandler)&loadend);
    LR2CSVExecutor::AddHandler("#SCRATCHSIDE", (LR2CSVCommandHandler)&scratchside);
  }
};

// register handler
LR2CSVPlaySceneHandlers _LR2CSVPlaySceneHandlers;

}
