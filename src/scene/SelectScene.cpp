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

  // Add select data
  MakeSelectDataList();

  Scene::LoadScene();

  // Create and load wheel metric wheel metric is not exist
  Metric *metric = Setting::GetThemeMetricList().get_metric(get_name());
  if (!Setting::GetThemeMetricList().get_metric("MusicWheel"))
    wheel_.Load(*metric);
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

  if (e.type() == InputEvents::kOnKeyDown || e.type() == InputEvents::kOnKeyPress)
  {
    if (e.KeyCode() == GLFW_KEY_UP)
    {
      wheel_.NavigateUp();
      SongList::getInstance().select(wheel_.get_selected_data(0).index);
      EventManager::SendEvent("SongSelectChanged");
    }
    else if (e.KeyCode() == GLFW_KEY_DOWN)
    {
      wheel_.NavigateDown();
      SongList::getInstance().select(wheel_.get_selected_data(0).index);
      EventManager::SendEvent("SongSelectChanged");
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

void SelectScene::MakeSelectDataList()
{
  int i = 0;
  for (auto &song : SongList::getInstance())
  {
    auto &item = *wheel_.NewData<MusicWheelData>();
    item.title = song.title;
    item.artist = song.artist;
    item.subtitle = song.subtitle;
    item.subartist = song.subartist;
    item.songpath = song.songpath;
    item.chartname = song.chartpath;
    item.type = 0;
    item.level = song.level;
    item.index = i++;
  }

  if (wheel_.size() == 0)
  {
    auto &item = *wheel_.NewData<MusicWheelData>();
    item.title = "(No Song)";
    item.index = -1;
  }
}

}