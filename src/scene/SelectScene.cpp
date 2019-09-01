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

  Scene::LoadScene();

  // TODO: set SelectWheel properties from Theme properties
  //

  // Add select data
  // XXX: Test code
  wheel_.PushData({ 0, "TestSong1", "Art1" });
  wheel_.PushData({ 1, "TestSong2", "Art2" });
  wheel_.PushData({ 2, "TestSong3", "Art3" });
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
  // Check and process for some exclusive commands first
  if (prop_name == "#DST_BAR_BODY_ON" ||
      prop_name == "#DST_BAR_BODY_OFF" ||
      prop_name == "#SRC_BAR_BODY")
  {
    // add to rendering
    if (std::find(children_.begin(), children_.end(), &wheel_) == children_.end())
      children_.push_back(&wheel_);
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