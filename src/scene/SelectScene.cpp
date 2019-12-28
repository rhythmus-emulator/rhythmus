#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"
#include "Player.h"
#include "Song.h"

namespace rhythmus
{


// -------------------------- class SelectScene

SelectScene::SelectScene()
{
  set_name("SelectScene");
  next_scene_ = "PlayScene";
  prev_scene_ = "Exit";
}

void SelectScene::LoadScene()
{
  // Before starting, unload song.
  FOR_EACH_PLAYER(p, i)
  {
    p->ClearPlayContext();
  }
  END_EACH_PLAYER()
  SongResource::getInstance().Clear();

  // Add wheel children first, as scene parameter may need it.
  // (e.g. LR2 command)
  AddChild(&wheel_);

  Scene::LoadScene();
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
  if (e.type() == InputEvents::kOnKeyUp && e.KeyCode() == GLFW_KEY_ESCAPE)
  {
    // Exit program instantly.
    CloseScene(false);
    return;
  }

  if (!is_input_available())
    return;

  Scene::ProcessInputEvent(e);

  if (e.type() == InputEvents::kOnKeyDown || e.type() == InputEvents::kOnKeyPress)
  {
    if (e.KeyCode() == GLFW_KEY_UP)
    {
      wheel_.NavigateUp();
      SongList::getInstance().select(wheel_.get_selected_data(0).index);
    }
    else if (e.KeyCode() == GLFW_KEY_DOWN)
    {
      wheel_.NavigateDown();
      SongList::getInstance().select(wheel_.get_selected_data(0).index);
    }
  }

  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == GLFW_KEY_ENTER)
    {
      // Register select song / course into Game / Player state.
      // XXX: can we preload selected song from here before PlayScene...?
      // XXX: what about course selection? select in gamemode?
      FOR_EACH_PLAYER(p, i)
      {
        auto &d = wheel_.get_selected_data(i);
        if (i == 1) /* main player */
          Game::getInstance().push_song(d.songpath);
        p->AddChartnameToPlay(d.chartname);
      }
      END_EACH_PLAYER()

      // Song selection - immediately change scene mode
      CloseScene(true);
    }
  }
}

}