#include "SelectWheel.h"
#include "Game.h"

namespace rhythmus
{

constexpr int kScrollPosMaxDiff = 3;
constexpr int kDefaultBarCount = 30;

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
  : curve_level_(0), bar_height_(40), bar_margin_(4),
    bar_offset_x_(0), text_margin_x_(60), text_margin_y_(2),
    bar_pos_method_(kBarPosExpression),
    focused_index_(0), center_index_(0), scroll_pos_(0)
{
  memset(select_bar_src_, 0, sizeof(select_bar_src_));
  SetBarPosition(-360, 2);
}

SelectWheel::~SelectWheel()
{
  // same as releasing all objects
  Prepare(0);
}

void SelectWheel::SetCenterIndex(int center_idx)
{
  center_index_ = center_idx;
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
  bar_center_y_ = Game::getInstance().get_window_height() / 2;
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

void SelectWheel::PushData(const SelectItemData& data)
{
  select_data_.push_back(data);
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
  {
    bar_.push_back(new SelectItem());
    bar_pos_fixed_.push_back(nullptr);
  }
  // bar_pos can contain one more element
  bar_pos_fixed_.push_back(nullptr);
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
    SetItemPosByIndex(item, idx_pos, r);
  }
}

/**
 * idx : actual
 */
void SelectWheel::SetItemPosByIndex(SelectItem& item, int idx, double r)
{
  switch (bar_pos_method_)
  {
  case SelectBarPosMethod::kBarPosExpression:
  {
    int idx_by_center = idx - center_index_;
    item.SetPos(bar_offset_x_ + 10,
      bar_center_y_ + (bar_height_ + bar_margin_) * idx_by_center - bar_height_ * 0.5f
    );
    break;
  }
  case SelectBarPosMethod::kBarPosFixed:
  {
    // over index size - cannot set position
    if (idx > bar_pos_fixed_.size())
    {
      item.Hide();
      return;
    }
    item.Show();
    BaseObject *obj1, *obj2;
    obj1 = bar_pos_fixed_[idx];
    obj2 = bar_pos_fixed_[idx + 1];
    if (!obj1 || !obj2)
    {
      item.Hide();
      return;
    }
    DrawProperty d;
    MakeTween(d,
      obj1->get_draw_property(),
      obj2->get_draw_property(),
      r, EaseTypes::kEaseLinear);
    item.img_sprite_->LoadDrawProperty(d);
    item.img_sprite_->Show();

    break;
  }
  }
}

void SelectWheel::RebuildItems()
{
  size_t disp_cnt = bar_.size();

  // reconstruct children - delete previous children
  for (auto i = 0; i < disp_cnt; ++i)
    RemoveChild(bar_[i]->img_sprite_.get());

  // now create new child.
  for (auto i = 0; i < disp_cnt; ++i)
  {
    int dataindex = mod_pos(focused_index_ + i, select_data_.size());
    int barindex = (focused_index_ + i) % disp_cnt;
    auto& data = select_data_[dataindex];
    auto& bar = *bar_[barindex];

    if (select_bar_src_[data.type] != nullptr)
    {
      bar.img_sprite_ =
        std::make_unique<Sprite>(*select_bar_src_[data.type]);
      bar.img_sprite_->set_name("SELITEM_" + std::to_string(i)); // XXX: for debug
    }
    else
    {
      bar.img_sprite_.reset();
      continue;
    }
    bar.text_sprite_->SetText(data.name);
    bar.dataindex = dataindex;
    bar.barindex = i;

    // add to child to render
    AddChild(bar.img_sprite_.get());
  }
}

void SelectWheel::SetBarPositionMethod(int use_expr)
{
  bar_pos_method_ = use_expr;
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

void SelectWheel::doUpdate(float delta)
{
  if (-delta < scroll_pos_ && scroll_pos_ < delta)
    scroll_pos_ = 0;
  else if (scroll_pos_ < 0)
    scroll_pos_ += delta / 1000.0f;
  else if (scroll_pos_ > 0)
    scroll_pos_ -= delta / 1000.0f;
  UpdateItemPos();
}

void SelectWheel::doRender()
{

}

void SelectWheel::LoadProperty(const std::string& prop_name, const std::string& value)
{
  /* LR2 type commands */
  if (prop_name == "#DST_BAR_BODY_ON")
  {
    if (bar_.empty()) Prepare(kDefaultBarCount);
    // TODO
  }
  else if (prop_name == "#DST_BAR_BODY_OFF")
  {
    // if not initialized, then do it
    if (bar_.empty()) Prepare(kDefaultBarCount);

    // set pos type automatically
    SetBarPositionMethod(SelectBarPosMethod::kBarPosFixed);

    std::vector<std::string> params;
    ConvertFromString(value, params);
    int attr = atoi(params[0].c_str());
    if (attr < 0 || attr > kDefaultBarCount) return;

    if (!bar_pos_fixed_[attr])
    {
      auto *o = new BaseObject();
      o->set_name("DST_BAR_BODY_OFF_" + std::to_string(attr));
      bar_pos_fixed_[attr] = o;
      RegisterChild(o);
      AddChild(o);    // not for rendering but for updating
    }
    bar_pos_fixed_[attr]->LoadProperty(prop_name, value);
  }
  else if (prop_name == "#SRC_BAR_BODY")
  {
    std::vector<std::string> params;
    ConvertFromString(value, params);
    int attr = atoi(params[0].c_str());
    if (attr < 0 || attr >= 30) return;

    if (!select_bar_src_[attr])
    {
      select_bar_src_[attr] = new Sprite();
      RegisterChild(select_bar_src_[attr]);
      AddChild(select_bar_src_[attr]);  // not for rendering but for updating
      select_bar_src_[attr]->Hide();
    }
    select_bar_src_[attr]->LoadProperty(prop_name, value);
  }
  // TODO: BAR_LEVEL, BAR_TEXT, ...
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


}