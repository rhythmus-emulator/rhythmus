#include "ListView.h"
#include "Util.h"
#include "common.h"

namespace rhythmus
{

// kinda trick to return always positive value when modulous.
inline int mod_pos(int a, int b)
{
  return ((a % b) + b) % b;
}

inline float modf_pos(float a, float b)
{
  return fmod(fmod(a, b) + b, b);
}

// ---------------------------- class ListViewItem

ListViewItem::ListViewItem()
  : dataindex_(0), data_(nullptr), is_focused_(false)
{
}

void ListViewItem::LoadFromData(void *data)
{
  data_ = data;
}

void ListViewItem::set_dataindex(int dataindex)
{
  dataindex_ = dataindex;
}

int ListViewItem::get_dataindex() const
{
  return dataindex_;
}

void ListViewItem::set_focus(bool focused)
{
  is_focused_ = focused;
}

void* ListViewItem::get_data()
{
  return data_;
}

// --------------------------------- class Menu

ListView::ListView()
  : data_index_(0),
    display_count_(16), focus_index_(8), focus_min_(4), focus_max_(12),
    scroll_time_(200.0f), scroll_time_remain_(.0f),
    scroll_delta_(.0f), scroll_delta_init_(.0f), inf_scroll_(false),
    pos_method_(MenuPosMethod::kMenuPosExpression)
{
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));

  // Set bar position expr param with default
  pos_expr_param_.bar_offset_x = GRAPHIC->width() - 540;
  pos_expr_param_.bar_center_y = GRAPHIC->height() / 2;
  pos_expr_param_.bar_height = 40;
  pos_expr_param_.bar_width = 540;
  pos_expr_param_.bar_margin = 1;
  pos_expr_param_.curve_size = 80;
  pos_expr_param_.curve_level = 5;
}

ListView::~ListView()
{
  Clear();
  for (auto* p : items_)
  {
    RemoveChild(p);
    delete p;
  }
  items_.clear();
}

void ListView::Load(const MetricGroup &metric)
{
  BaseObject::Load(metric);

  pos_method_ = metric.get<int>("PositionType");
  int center_index = metric.get<int>("CenterIndex");
  if (center_index)
  {
    set_focus_max_index(center_index);
    set_focus_min_index(center_index);
    set_focus_index(center_index);
  }
  display_count_ = metric.get<int>("BarCount");

  // Build items (bar item)
  while (items_.size() < (unsigned)display_count_ + kScrollPosMaxDiff * 2)
  {
    auto *item = CreateMenuItem();
    item->set_parent(this);
    item->Load(metric);
    item->Hide();
    AddChild(item);
    items_.push_back(item);
  }

  // Set item position
  for (int i = 0; i < display_count_; ++i)
  {
    pos_fixed_param_.tween_bar[i].set_name(format_string("Bar%d", i));
    pos_fixed_param_.tween_bar_focus[i].set_name(format_string("BarOn%d", i));
    pos_fixed_param_.tween_bar[i].Hide();
    pos_fixed_param_.tween_bar_focus[i].Hide();
    AddChild(&pos_fixed_param_.tween_bar[i]);
    AddChild(&pos_fixed_param_.tween_bar_focus[i]);

    pos_fixed_param_.tween_bar[i].Load(metric);
  }

  // Load sounds
  std::string soundpath;
  if (METRIC->get_safe("Sound.SongSelectChange", soundpath))
    wheel_sound_.Load(soundpath);

  // Build data & item
  RebuildData();

  // selectchange for initiailize.
  OnSelectChange(data_[data_index_], 0);
}

void ListView::AddData(void* d) { data_.push_back(d); }
void* ListView::GetSelectedMenuData() { return GetMenuDataByIndex(data_index_); }
void* ListView::GetMenuDataByIndex(int index) { return items_[index]; }

void ListView::SelectMenuByIndex(int index)
{
  data_index_ = index;
  RebuildItems();
}

int ListView::size() const { return data_.size(); }
int ListView::index() const { return data_index_; }

void ListView::Clear()
{
  /* unreference data from bar items. */
  for (auto* p : items_)
    p->LoadFromData(nullptr);
  /* now delete all data. */
  for (auto* p : data_)
    delete p;
  data_.clear();
}

void ListView::set_display_count(int display_count)
{
  display_count_ = display_count;
}

void ListView::set_focus_min_index(int min_index)
{
  focus_min_ = min_index;
}

void ListView::set_focus_max_index(int max_index)
{
  focus_max_ = max_index;
}

void ListView::set_focus_index(int center_idx)
{
  focus_index_ = center_idx;
}

void ListView::set_infinite_scroll(bool inf_scroll)
{
  inf_scroll_ = inf_scroll;
}

void ListView::RebuildData()
{}

void ListView::RebuildItems()
{
  /* Number dataindex for all items. */
  auto data_count = size();
  R_ASSERT(data_count > 0);
  int data_idx = data_index_ - focus_index_ - kScrollPosMaxDiff;
  data_idx %= data_count;
  if (data_idx < 0) data_idx += data_count;
  for (auto *item : items_)
  {
    void* cur_data = data_[data_idx];
    item->set_dataindex(data_idx);
    item->LoadFromData(cur_data);
    data_idx = (data_idx + 1) % data_count;
  }
}

void ListView::NavigateDown()
{
  if (!inf_scroll_ && data_index_ >= size() - 1)
    return;

  wheel_sound_.Play();

  // change data index and scroll effect (set time / delta)
  data_index_ = (data_index_ + 1) % size();
  scroll_delta_init_ = scroll_delta_ - 1.0f;
  if (scroll_delta_init_ < -kScrollPosMaxDiff)
    scroll_delta_init_ = -kScrollPosMaxDiff;
  scroll_time_remain_ = scroll_time_;

  // shift up item
  /*
  int new_dataindex = (bar_.back()->get_dataindex() + 1) % size();
  std::rotate(bar_.begin(), std::prev(bar_.end()), bar_.end());
  bar_.back()->set_dataindex(new_dataindex);
  */
  std::rotate(items_.begin(), std::prev(items_.end()), items_.end());

  // update items
  RebuildItems();

  OnSelectChange(data_[data_index_], 1);
}

void ListView::NavigateUp()
{
  if (!inf_scroll_ && data_index_ == 0)
    return;

  wheel_sound_.Play();

  // change data index and scroll effect (set time / delta)
  data_index_--;
  if (data_index_ < 0) data_index_ += size();
  scroll_delta_init_ = scroll_delta_ + 1.0f;
  if (scroll_delta_init_ > kScrollPosMaxDiff)
    scroll_delta_init_ = kScrollPosMaxDiff;
  scroll_time_remain_ = scroll_time_;

  // shift down item
  /*
  int new_dataindex = bar_.front()->get_dataindex();
  std::rotate(bar_.begin(), std::next(bar_.begin()), bar_.end());
  bar_.front()->set_dataindex(new_dataindex);
  */
  std::rotate(items_.begin(), std::next(items_.begin()), items_.end());

  // update items
  RebuildItems();

  OnSelectChange(data_[data_index_], -1);
}

void ListView::NavigateLeft() {}

void ListView::NavigateRight() {}

void ListView::doUpdate(double delta)
{
  // Update scroll pos
  scroll_time_remain_ = std::max(.0f, scroll_time_remain_ - (float)delta);
  scroll_delta_ = scroll_delta_init_ * std::pow(scroll_time_remain_ / scroll_time_, 2);
  if (scroll_delta_init_ != 0 && scroll_delta_ == 0)
  {
    scroll_delta_init_ = 0;
    OnSelectChanged();
  }

  // calculate each bar position-based-index and position
  UpdateItemPos();
}

void ListView::UpdateItemPos()
{
  switch (pos_method_)
  {
  case MenuPosMethod::kMenuPosExpression:
    UpdateItemPosByExpr();
    break;
  case MenuPosMethod::kMenuPosFixed:
    UpdateItemPosByFixed();
    break;
  }
}

void ListView::UpdateItemPosByExpr()
{
  int x, y, i;
  int display_count = (int)items_.size();

  // firstly - set all object's position
  for (i = 0; i < display_count; ++i)
  {
    double pos = i - display_count / 2 + scroll_delta_;
    x = static_cast<int>(
      pos_expr_param_.bar_offset_x + pos_expr_param_.curve_size *
      (1.0 - cos(pos / pos_expr_param_.curve_level))
      );
    y = static_cast<int>(
      pos_expr_param_.bar_center_y +
      (pos_expr_param_.bar_height + pos_expr_param_.bar_margin) * pos -
      pos_expr_param_.bar_height * 0.5f
      );
    items_[i]->SetPos(x, y);
    items_[i]->SetSize(pos_expr_param_.bar_width, pos_expr_param_.bar_height);
  }

  // decide range of object to show
  int start_idx_show = kScrollPosMaxDiff +  + (int)floor(scroll_delta_);
  int end_idx_show = display_count - kScrollPosMaxDiff + (int)ceil(scroll_delta_);
  for (i = 0; i < display_count; ++i)
  {
    if (i < start_idx_show || i > end_idx_show)
      items_[i]->Hide();
    else
      items_[i]->Show();
  }
}

void ListView::UpdateItemPosByFixed()
{
  int scroll_offset = (int)floor(scroll_delta_);
  float ratio = 1.0f - (scroll_delta_ - scroll_offset);
  int ii = 0;
  DrawProperty d;

  /* Hide all elements first. */
  for (int i = 0; i < display_count_ + kScrollPosMaxDiff * 2; ++i)
  {
    items_[i]->Hide();
  }

  /* Reset item position */
  for (int i = 0; i < display_count_ - 1; ++i)
  {
    /* TODO: skip if data index is over when inf_scroll is off. */
    int ii = i + kScrollPosMaxDiff + scroll_offset + 1;
    ii %= display_count_ + kScrollPosMaxDiff * 2;
    BaseObject *obj1 = &pos_fixed_param_.tween_bar[i];
    BaseObject *obj2 = &pos_fixed_param_.tween_bar[i + 1];
    MakeTween(d,
      obj1->GetCurrentFrame(),
      obj2->GetCurrentFrame(),
      ratio, EaseTypes::kEaseLinear);
    items_[ii]->SetPos((int)d.pos.x, (int)d.pos.y);
    items_[ii]->Show();
  }
}

ListViewItem* ListView::CreateMenuItem()
{
  return new ListViewItem();
}

void ListView::OnSelectChange(const void *data, int direction) {}

void ListView::OnSelectChanged() {}

}
