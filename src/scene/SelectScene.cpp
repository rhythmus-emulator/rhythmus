#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"
#include "Song.h"

namespace rhythmus
{


// -------------------------- class SelectScene

SelectScene::SelectScene()
{
  set_name("SelectScene");
}

void SelectScene::LoadScene()
{
  // TODO: place this code to Game setting
  Game::getInstance().SetAttribute(
    "SelectScene", "../themes/WMIX_HD/select/select.lr2skin"
  );

  // Add wheel children first, as scene parameter may need it.
  // (e.g. LR2 command)
  AddChild(&wheel_);

  Scene::LoadScene();

  // TODO: set SelectWheel properties from Theme properties
  //

  // Add select data
  MakeSelectDataList();

  // Should build wheel items
  wheel_.RebuildItems();

  // Send some initial events to invalidate specific object
  // (e.g. song title text object)
  // - should be called AFTER wheel / LR2objects(LoadScene) are loaded.
  EventManager::SendEvent(Events::kEventSongSelectChanged);
}

void SelectScene::StartScene()
{
  Scene::StartScene();

  // TODO
}

void SelectScene::CloseScene()
{
  Scene::CloseScene();

  // TODO
}

bool SelectScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsInput() && !IsEventValidTime(e))
    return true;

  if (e.IsKeyDown() || e.IsKeyPress())
  {
    if (e.GetKeycode() == GLFW_KEY_UP)
    {
      wheel_.ScrollUp();
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
    else if (e.GetKeycode() == GLFW_KEY_DOWN)
    {
      wheel_.ScrollDown();
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
  }

  if (e.IsKeyUp())
  {
    if (e.GetKeycode() == GLFW_KEY_ESCAPE)
    {
      Graphic::getInstance().ExitRendering();
    }
    else if (e.GetKeycode() == GLFW_KEY_ENTER)
    {
      // Song selection - immediately change scene mode
      Game::getInstance().SetNextGameMode(GameMode::kGameModeDecide);
      Game::getInstance().ChangeGameMode();
    }
    else if (e.GetKeycode() == GLFW_KEY_A)
    {
      TriggerFadeIn(1000);
    }
    else if (e.GetKeycode() == GLFW_KEY_S)
    {
      TriggerFadeOut(1000);
    }
  }

  return true;
}

void SelectScene::LoadProperty(const std::string& prop_name, const std::string& value)
{
  // Check and process for LR2 exclusive commands
  if (prop_name.compare(0, 9, "#DST_BAR_") == 0 ||
      prop_name.compare(0, 9, "#SRC_BAR_") == 0)
  {
    // change order: move wheel to end
    auto it = std::find(children_.begin(), children_.end(), &wheel_);
    ASSERT(it != children_.end());
    std::rotate(it, it + 1, children_.end());
    // set property
    wheel_.LoadProperty(prop_name, value);
    return;
  }
  else if (prop_name == "#BAR_CENTER")
  {
    wheel_.LoadProperty(prop_name, value);
    return;
  }

  Scene::LoadProperty(prop_name, value);
}

MusicWheel& SelectScene::get_wheel()
{
  return wheel_;
}

void SelectScene::MakeSelectDataList()
{
#if 0
  // XXX: Test code
  wheel_.AddData(new MusicWheelItemData{ "TestSong1", "sub", "gen", "Art1", "", 0, 10, nullptr });
  wheel_.AddData(new MusicWheelItemData{ "TestSong2", "sub", "gen", "Art2", "", 1, 10, nullptr });
  wheel_.AddData(new MusicWheelItemData{ "TestSong3", "sub", "gen", "Art3", "", 2, 11, nullptr });
#endif
  
  for (auto &song : SongList::getInstance())
  {
    int cnt = song->GetChartCount();
    for (int i = 0; i < cnt; ++i)
    {
      auto &item = wheel_.NewData();
      rparser::Chart* chart = song->GetChart(i);
      auto &meta = chart->GetMetaData();
      meta.SetMetaFromAttribute();
      meta.SetUtf8Encoding();
      item.title = meta.title;
      item.artist = meta.artist;
      item.type = 0;
      item.level = meta.level;
      item.song = nullptr;
      song->CloseChart();
    }
  }

  if (wheel_.get_data_size() == 0)
  {
    wheel_.AddData(new MusicWheelItemData{ "(No Song)", "-", "-", "-", "", 0, 0, nullptr });
  }
}

}