#include "SelectScene.h"
#include "Event.h"
#include "SceneManager.h"

namespace rhythmus
{

constexpr int kScrollPosMaxDiff = 3;

// kinda trick to return always positive value when modulous.
inline int mod_pos(int a, int b)
{
  return ((a % b) + a) % b;
}


// -------------------- class SelectBarPosition

SelectBarPosition::SelectBarPosition()
  : center_idx_(0), curve_level_(0), bar_height_(40), bar_margin_(4),
    bar_offset_x_(0), text_margin_x_(60), text_margin_y_(2)
{
  SetBarPosition(-360, 2);
}

void SelectBarPosition::SetCenterIndex(int center_idx)
{
  center_idx_ = center_idx;
}

void SelectBarPosition::SetCurveLevel(double curve_level)
{
  curve_level_ = curve_level;
}

void SelectBarPosition::SetBarPosition(int offset, int align)
{
  switch (align)
  {
  case 0:
    bar_offset_x_ = 0;
    break;
  case 1:
    bar_offset_x_ = Game::getInstance().get_window_width() / 2;
    break;
  case 2:
    bar_offset_x_ = Game::getInstance().get_window_width();
    break;
  }

  bar_offset_x_ += offset;
  curve_size_ = -offset;
}

void SelectBarPosition::SetBarHeight(int height)
{
  bar_height_ = height;
}

void SelectBarPosition::SetBarMargin(int margin)
{
  bar_margin_ = margin;
}

void SelectBarPosition::SetTextMargin(double x, double y)
{
  text_margin_x_ = x;
  text_margin_y_ = y;
}

// TODO: use 'GetPos' instead of TweenInfo, and use LoadPos() in TweenInfo
void SelectBarPosition::GetTweenInfo(double pos_idx, TweenInfo &out)
{
  double actual_index = pos_idx - center_idx_;
  double x_offset_ = 1.0 - cos(pos_idx / 100 * curve_level_);

  out.x = bar_offset_x_ + x_offset_ * curve_size_;
  out.y = (bar_height_ + bar_margin_) * actual_index - bar_height_ / 2;

  Game::getInstance().get_window_height();
}

void SelectBarPosition::GetTextTweenInfo(double pos_idx, TweenInfo &out)
{
  GetTweenInfo(pos_idx, out);
  out.pi.x += text_margin_x_;
  out.pi.y += text_margin_y_;
}


// -------------------------- class SelectScene

SelectScene::SelectScene()
  : focused_index_(0), center_index_(0), scroll_pos_(.0),
    select_bar_draw_count_(NUM_SELECT_BAR_DISPLAY_MAX)
{
}

void SelectScene::StartScene()
{
  // Load theme matrixes
  // TODO

  InitializeItems();

  // Add select data
  // XXX: Test code
  select_data_.push_back({ 0, "TestSong1", "Art1" });
  select_data_.push_back({ 1, "TestSong2", "Art2" });
  select_data_.push_back({ 2, "TestSong3", "Art3" });
  focused_index_ = 0;
}

void SelectScene::CloseScene()
{
  // TODO
}

void SelectScene::Update()
{
  UpdateItemPos();

  for (const auto& spr : sprites_)
  {
    spr->Update();
  }
}
void SelectScene::Render()
{
  for (const auto& spr : sprites_)
  {
    spr->Render();
  }
}

bool SelectScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyDown() || e.IsKeyPress())
  {
    if (e.GetKeycode() == GLFW_KEY_UP)
    {
      focused_index_--;
      if (focused_index_ < 0) focused_index_ += select_data_.size();
      scroll_pos_ += 1;
      if (scroll_pos_ > kScrollPosMaxDiff)
        scroll_pos_ = kScrollPosMaxDiff;
      RebuildItems();
      EventManager::SendEvent(Events::kEventSongSelectChanged);
    }
    else if (e.GetKeycode() == GLFW_KEY_DOWN)
    {
      focused_index_ = (focused_index_) % select_data_.size();
      scroll_pos_ -= -1;
      if (scroll_pos_ < -kScrollPosMaxDiff)
        scroll_pos_ = -kScrollPosMaxDiff;
      RebuildItems();
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
  return select_data_[get_selected_index()].name.c_str();
}

int SelectScene::get_selected_index() const
{
  return focused_index_;
}

const char* SelectScene::get_select_bar_title(int select_bar_idx) const
{
  int curr_idx_ = static_cast<int>(scroll_pos_);
  return select_data_[(curr_idx_ + select_bar_idx) % select_data_.size()]
    .name.c_str();
}

/* Get selection scroll. */
double SelectScene::get_select_bar_scroll() const
{
  return scroll_pos_;
}

void SelectScene::SetSelectBarPosition(SelectBarPosition* position_object)
{
  // use basic bar position calculator if not given
  if (!position_object)
    bar_position_ = std::make_unique<SelectBarPosition>();
  else
    bar_position_.reset(position_object);
}

SelectBarPosition* SelectScene::GetSelectBarPosition()
{
  return bar_position_.get();
}

void SelectScene::SetSelectBarImage(int type_no, ImageAuto img)
{
  if (type_no < 0 || type_no >= NUM_SELECT_BAR_TYPES)
    return;

  select_bar_src_[type_no]->SetImage(img);
}

void SelectScene::UpdateItemPos()
{
  // Update scroll pos
  double scroll_diff_ = scroll_pos_ * 0.3;
  if (scroll_diff_ < 0 && scroll_diff_ > -0.1) scroll_diff_ = 0.1;
  else if (scroll_diff_ > 0 && scroll_diff_ < 0.1) scroll_diff_ = 0.1;
  scroll_pos_ += scroll_diff_;

  // calculate each bar position-based-index and position
  if (bar_position_)
  {;
    double idx_pos_d = 0;
    int idx_pos = 0;
    double r = scroll_pos_ - floor(scroll_pos_);

    for (int i = 0; i < select_bar_draw_count_; ++i)
    {
      auto& item = select_bar_[i];
      idx_pos_d = item.barindex + center_index_ + scroll_pos_;
      if (idx_pos_d < 0) idx_pos_d += select_bar_draw_count_;
      idx_pos = static_cast<int>(idx_pos_d);

      Sprite* bar_item = item.img_sprite_.get();
      Sprite* bar_text = item.img_sprite_.get();
      TweenInfo ti;
      bar_position_->GetTweenInfo(idx_pos_d, ti);
      bar_item->get_animation().LoadTweenCurr(ti);
      bar_position_->GetTextTweenInfo(idx_pos_d, ti);
      bar_text->get_animation().LoadTweenCurr(ti);
    }
  }

#if 0
  // Update animation of each bar position
  for (auto& ani : select_bar_pos_)
  {
    ani.Tick(SceneManager::GetSceneTickTime());
  }

  // calculate each bar position-based-index and position
  double r = 0;
  double idx_pos_d = 0;
  int idx_pos = 0;
  for (int i = 0; i < select_bar_draw_count_; ++i)
  {
    auto& item = select_bar_[i];
    idx_pos_d = item.barindex + center_index_ + scroll_pos_;
    if (idx_pos_d < 0) idx_pos_d += select_bar_draw_count_;
    idx_pos = static_cast<int>(idx_pos_d);
    r = idx_pos_d - idx_pos;
    
    if (idx_pos >= select_bar_draw_count_ - 1) continue;
    const TweenInfo& t1 = select_bar_pos_[idx_pos].GetCurrentTweenInfo();
    const TweenInfo& t2 = select_bar_pos_[idx_pos + 1].GetCurrentTweenInfo();
    TweenInfo t;
    MakeTween(t, t1, t2, r);

    Sprite* bar_item = item.img_sprite_.get();
    Sprite* bar_text = item.img_sprite_.get();
    bar_item->get_animation().LoadTweenCurr(t);
    bar_text->get_animation().LoadTweenCurr(t);
    bar_text->get_animation().MovePosition(70, 2);  // TODO: use theme animation object again
  }
#endif
}

// Reserve bar items as much as required
void SelectScene::InitializeItems()
{
  for (int i = 0; i < select_bar_draw_count_; ++i)
  {
    select_bar_.emplace_back(SelectItem{
      std::make_unique<Sprite>(),
      std::make_unique<Text>(),
      i
    });
  }
}

void SelectScene::RebuildItems()
{
  for (int i = 0; i < select_bar_draw_count_; ++i)
  {
    int dataindex = mod_pos( focused_index_ + i, select_data_.size() );
    int barindex = (focused_index_ + i) % select_bar_draw_count_;
    auto& data = select_data_[dataindex];
    auto& bar = select_bar_[barindex];

    bar.img_sprite_ =
      std::make_unique<Sprite>(*select_bar_src_[data.type]);
    bar.text_sprite_->SetText(data.name);
    bar.dataindex = dataindex;
    bar.barindex = i;
  }
}

void SelectScene::DrawSelectBar()
{
  for (int i = 0; i < select_bar_draw_count_; ++i)
  {
    auto& item = select_bar_[i];
    item.img_sprite_->Render();
    item.text_sprite_->Render();
  }
}

int SelectScene::get_select_list_size() const
{
  return select_data_.size();
}

}