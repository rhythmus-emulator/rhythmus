#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"
#include "Player.h"
#include "Song.h"
#include "SongPlayer.h"

namespace rhythmus
{


// -------------------------- class SelectScene

SelectScene::SelectScene()
{
  set_name("SelectScene");
  next_scene_ = "DecideScene";
  prev_scene_ = "Exit";
}

void SelectScene::LoadScene()
{
  // Before starting, unload song.
  SongPlayer::getInstance().Stop();
  PlayerManager::CreateGuestPlayerIfEmpty();

  RegisterPredefObject(&wheel_);
  AddChild(&wheel_);

  Scene::LoadScene();
}

void SelectScene::StartScene()
{
  Scene::StartScene();
  EVENTMAN->SendEvent("SelectSceneLoad");
}

void SelectScene::CloseScene(bool next)
{
  if (next)
  {
    // push selected song info to SongResource & Players for playing.

  }

  Scene::CloseScene(next);
}

void SelectScene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp && e.KeyCode() == RI_KEY_ESCAPE)
  {
    // Exit program instantly.
    CloseScene(false);
    return;
  }

  Scene::ProcessInputEvent(e);

  if (e.type() == InputEvents::kOnKeyDown || e.type() == InputEvents::kOnKeyPress)
  {
    if (e.KeyCode() == RI_KEY_UP)
    {
      wheel_.NavigateUp();
    }
    else if (e.KeyCode() == RI_KEY_DOWN)
    {
      wheel_.NavigateDown();
    }
    else if (e.KeyCode() == RI_KEY_RIGHT)
    {
      wheel_.NavigateRight();
    }
    else if (e.KeyCode() == RI_KEY_LEFT)
    {
      wheel_.NavigateLeft();
    }
  }

  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == RI_KEY_ENTER)
    {
      // Register select song / course into Game / Player state.
      // XXX: can we preload selected song from here before PlayScene...?
      // XXX: what about course selection? select in gamemode?
      auto *d = wheel_.get_selected_data(0);
      R_ASSERT(d);
#if 0
      Game::getInstance().push_song(d.info.songpath);
      FOR_EACH_PLAYER(p, i)
      {
        // TODO: different chart selection for each player
        p->AddChartnameToPlay(d.name);
      }
      END_EACH_PLAYER()
#endif

      // TODO: use SongId for playlist queue
      SongPlayer::getInstance().AddSongtoPlaylist(
        d->GetChart()->songpath, d->GetChart()->chartpath
      );

      // Song selection - immediately change scene mode
      CloseScene(true);
    }
  }
}

}