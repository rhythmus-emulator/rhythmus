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


SelectItem::SelectItem()
{
  img_sprite_ = std::make_unique<Sprite>();
  text_sprite_ = std::make_unique<Text>();
  AddChild(img_sprite_.get());
  AddChild(text_sprite_.get());
}

// -------------------- class SelectBarPosition

SelectWheel::SelectWheel()
  : center_idx_(0), curve_level_(0), bar_height_(40), bar_margin_(4),
    bar_offset_x_(0), text_margin_x_(60), text_margin_y_(2)
{
  SetBarPosition(-360, 2);
}

SelectWheel::~SelectWheel()
{
  // same as releasing all objects
  Prepare(0);
}

void SelectWheel::SetCenterIndex(int center_idx)
{
  center_idx_ = center_idx;
}

void SelectWheel::SetCurveLevel(double curve_level)
{
  curve_level_ = curve_level;
}

void SelectWheel::SetBarPosition(int offset, int align)
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

void SelectWheel::SetBarHeight(int height)
{
  bar_height_ = height;
}

void SelectWheel::SetBarMargin(int margin)
{
  bar_margin_ = margin;
}

void SelectWheel::SetTextMargin(double x, double y)
{
  text_margin_x_ = x;
  text_margin_y_ = y;
}

size_t SelectWheel::get_select_list_size() const
{
  return select_data_.size();
}

/* Get selection scroll. */
double SelectWheel::get_select_bar_scroll() const
{
  return scroll_pos_;
}

int SelectWheel::get_selected_index() const
{
  return focused_index_;
}

const char* SelectWheel::get_selected_title() const
{
  return select_data_[get_selected_index()].name.c_str();
}

void SelectWheel::SetSelectBarImage(int type_no, ImageAuto img)
{
  if (type_no < 0 || type_no >= NUM_SELECT_BAR_TYPES)
    return;

  select_bar_src_[type_no]->SetImage(img);
}

void SelectWheel::Prepare(int visible_bar_count)
{
  for (auto *obj : bar_)
    delete obj;
  bar_.clear();

  for (int i = 0; i < visible_bar_count; ++i)
    bar_.push_back(new SelectItem());
}

void SelectWheel::UpdateItemPos()
{
  // Update scroll pos
  double scroll_diff_ = scroll_pos_ * 0.3;
  if (scroll_diff_ < 0 && scroll_diff_ > -0.1) scroll_diff_ = 0.1;
  else if (scroll_diff_ > 0 && scroll_diff_ < 0.1) scroll_diff_ = 0.1;
  scroll_pos_ += scroll_diff_;

  // calculate each bar position-based-index and position
  double idx_pos_d = 0;
  int idx_pos = 0;
  double r = scroll_pos_ - floor(scroll_pos_);
  size_t select_bar_draw_count_ = bar_.size();

  for (int i = 0; i < select_bar_draw_count_; ++i)
  {
    auto& item = *bar_[i];
    idx_pos_d = item.barindex + center_index_ + scroll_pos_;
    if (idx_pos_d < 0) idx_pos_d += select_bar_draw_count_;
    idx_pos = static_cast<int>(idx_pos_d);
    SetItemPosByIndex(item, i, r);
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

void SelectWheel::RebuildItems()
{
  size_t disp_cnt = bar_.size();
  for (auto i = 0; i < disp_cnt; ++i)
  {
    int dataindex = mod_pos(focused_index_ + i, select_data_.size());
    int barindex = (focused_index_ + i) % disp_cnt;
    auto& data = select_data_[dataindex];
    auto& bar = *bar_[barindex];

    bar.img_sprite_ =
      std::make_unique<Sprite>(*select_bar_src_[data.type]);
    bar.text_sprite_->SetText(data.name);
    bar.dataindex = dataindex;
    bar.barindex = i;
  }
}

void SelectWheel::ScrollDown()
{
  focused_index_ = (focused_index_) % select_data_.size();
  scroll_pos_ -= -1;
  if (scroll_pos_ < -kScrollPosMaxDiff)
    scroll_pos_ = -kScrollPosMaxDiff;
  RebuildItems();
}

void SelectWheel::ScrollUp()
{
  focused_index_--;
  if (focused_index_ < 0) focused_index_ += select_data_.size();
  scroll_pos_ += 1;
  if (scroll_pos_ > kScrollPosMaxDiff)
    scroll_pos_ = kScrollPosMaxDiff;
  RebuildItems();
}

void SelectWheel::SetItemPosByIndex(SelectItem& item, int idx, double r)
{
  // TODO
}

void SelectWheel::doUpdate()
{
  UpdateItemPos();
}

#if 0
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
#endif


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
  wheel_.select_data_.push_back({ 0, "TestSong1", "Art1" });
  wheel_.select_data_.push_back({ 1, "TestSong2", "Art2" });
  wheel_.select_data_.push_back({ 2, "TestSong3", "Art3" });
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

SelectWheel& SelectScene::get_wheel()
{
  return wheel_;
}

}