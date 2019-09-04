#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"

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
  // XXX: Test code
  wheel_.PushData({ 0, "TestSong1", "Art1" });
  wheel_.PushData({ 1, "TestSong2", "Art2" });
  wheel_.PushData({ 2, "TestSong3", "Art3" });

  // Should build wheel items
  wheel_.RebuildItems();
}

void SelectScene::StartScene()
{
  // TODO
}

void SelectScene::CloseScene()
{
  // TODO
}

bool SelectScene::ProcessEvent(const EventMessage& e)
{
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
  return true;
}

const std::string SelectScene::GetSceneName() const
{
  return "SelectScene";
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

  Scene::LoadProperty(prop_name, value);
}

SelectWheel& SelectScene::get_wheel()
{
  return wheel_;
}

}