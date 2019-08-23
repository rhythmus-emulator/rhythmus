#include "SelectScene.h"
#include "Event.h"

namespace rhythmus
{

void SelectScene::StartScene()
{
  // Test code
  select_list_.push_back({ "TestSong1", "Art1" });
  select_list_.push_back({ "TestSong2", "Art2" });
  select_list_.push_back({ "TestSong3", "Art3" });
  select_index_ = 0;
}

void SelectScene::CloseScene()
{
  // TODO
}

void SelectScene::Render()
{
  for (const auto& spr : sprites_)
  {
    spr->Update();
    spr->Render();
  }
}

bool SelectScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown())
  {
    if (e.GetKeycode() == GLFW_KEY_UP)
    {
      select_index_--;
      if (select_index_ < 0) select_index_ += select_list_.size();
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
    else if (e.GetKeycode() == GLFW_KEY_DOWN)
    {
      select_index_++;
      select_index_ %= select_list_.size();
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
  }
  return true;
}

const std::string SelectScene::GetSceneName() const
{
  return "SelectScene";
}

const char* SelectScene::get_selected_title() const
{
  return select_list_[select_index_].name.c_str();
}

}