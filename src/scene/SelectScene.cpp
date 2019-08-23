#include "SelectScene.h"
#include "Event.h"

namespace rhythmus
{

constexpr int kScrollPosMaxDiff = 3;

SelectScene::SelectScene()
  : select_index_(0), select_scroll_pos_(0), select_scroll_speed_(0)
{
}

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
  UpdateScrollPos();

  for (const auto& spr : sprites_)
  {
    spr->Update();
    spr->Render();
  }
}

bool SelectScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown() || e.IsKeyPress())
  {
    if (e.GetKeycode() == GLFW_KEY_UP)
    {
      select_index_--;
      if (select_index_ < select_scroll_pos_ - kScrollPosMaxDiff)
        select_scroll_pos_ = select_index_ + kScrollPosMaxDiff;
      select_scroll_speed_ = select_scroll_pos_ - select_index_;
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
    else if (e.GetKeycode() == GLFW_KEY_DOWN)
    {
      select_index_++;
      if (select_index_ > select_scroll_pos_ + kScrollPosMaxDiff)
        select_scroll_pos_ = select_index_ - kScrollPosMaxDiff;
      select_scroll_speed_ = select_scroll_pos_ - select_index_;
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
  return select_list_[get_selected_index()].name.c_str();
}

int SelectScene::get_selected_index() const
{
  const auto len = select_list_.size();

  // kinda trick to return always positive value.
  return (select_index_ % len + len) % len;
}

const char* SelectScene::get_select_bar_title(int select_bar_idx) const
{
  int curr_idx_ = static_cast<int>(select_scroll_pos_);
  return select_list_[(curr_idx_ + select_bar_idx) % select_list_.size()]
    .name.c_str();
}

/* Get selection scroll. */
double SelectScene::get_select_bar_scroll() const
{
  return select_scroll_pos_;
}

void SelectScene::UpdateScrollPos()
{
  if ((select_scroll_speed_ > 0 && select_scroll_pos_ > select_index_) ||
      (select_scroll_speed_ < 0 && select_scroll_pos_ < select_index_))
  {
    select_scroll_pos_ = select_index_;
    select_scroll_speed_ = 0;
  }
  select_scroll_pos_ += select_scroll_speed_ * Timer::GetGameTimeDelta();
}

int SelectScene::get_select_list_size() const
{
  return select_list_.size();
}

}