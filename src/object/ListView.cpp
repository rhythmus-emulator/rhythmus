#include "ListView.h"
#include "Util.h"
#include "common.h"

namespace rhythmus
{

// kinda trick to return always positive value when modulous.
static inline int mod_pos(int a, int b)
{
  return ((a % b) + b) % b;
}

static inline float modf_pos(float a, float b)
{
  return fmod(fmod(a, b) + b, b);
}

// ---------------------------- class ListViewItem

ListViewItem::ListViewItem() :
  dataindex_(0), data_(nullptr), is_focused_(false)
{
  own_children_ = true; // TODO: use BaseFrame()
}

ListViewItem::ListViewItem(const ListViewItem& obj) : BaseObject(obj),
  dataindex_(obj.dataindex_), data_(obj.data_), is_focused_(false),
  item_dprop_(obj.item_dprop_)
{
  own_children_ = true; // TODO: use BaseFrame()
}

BaseObject *ListViewItem::clone()
{
  return new ListViewItem(*this);
}

void ListViewItem::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  item_dprop_ = frame_;
}

void ListViewItem::LoadFromData(void *data)
{
  data_ = data;
}

void ListViewItem::set_dataindex(unsigned dataindex)
{
  dataindex_ = dataindex;
}

unsigned ListViewItem::get_dataindex() const
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

DrawProperty &ListViewItem::get_item_dprop()
{
  return item_dprop_;
}

void ListViewItem::OnAnimation(DrawProperty &frame)
{
  item_dprop_ = frame;
}

// @brief Test listviewitem for test purpose
class TestListViewItem : public ListViewItem
{
public:
private:
};

// --------------------------------- class Menu

ListView::ListView()
  : data_top_index_(0), data_index_(0), viewtype_(ListViewType::kList),
    item_count_(16), set_item_count_auto_(false), item_height_(80),
    item_center_index_(8), item_focus_min_(4), item_focus_max_(12),
    scroll_time_(200.0f), scroll_time_remain_(.0f), scroll_delta_(.0f),
    pos_method_(MenuPosMethod::kMenuPosExpression)
{
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));
  pos_expr_param_.curve_level = 1.0;
  own_children_ = true; // TODO: use BaseFrame()
}

ListView::~ListView()
{
  Clear();
}

void ListView::Load(const MetricGroup &metric)
{
  // Load itemview attributes
  BaseObject::Load(metric);
  metric.get_safe("postype", pos_method_);
  metric.get_safe("viewtype", (int&)viewtype_);
  if (viewtype_ >= ListViewType::kEnd)
    viewtype_ = ListViewType::kList;
  metric.get_safe("itemtype", item_type_);
  metric.get_safe("itemheight", item_height_);
  metric.get_safe("itemcountauto", set_item_count_auto_);
  if (!set_item_count_auto_)
    metric.get_safe("itemcount", item_count_);
  else
    item_count_ = CalculateItemCount();
  int center_index = -1;
  metric.get_safe("center_index", center_index);
  if (center_index >= 0)
  {
    set_item_min_index((unsigned)center_index);
    set_item_max_index((unsigned)center_index);
    set_item_center_index((unsigned)center_index);
  }

  // Create items
  const MetricGroup *itemmetric = metric.get_group("item");
  if (itemmetric)
  {
#if 1
    /* default: nullptr data */
    ListViewItem *item = CreateMenuItem(item_type_);
    item->Load(*itemmetric);
    item->LoadFromData(nullptr);
    item->set_dataindex(0);
    items_.push_back(item);
    AddChild(item);

    // clone
    // XXX: MAY NOT use clone() method to loading object!
    for (unsigned i = 1; i < item_count_; ++i)
    {
      ListViewItem *item = (ListViewItem*)items_[0]->clone();
      items_.push_back(item);
      item->set_dataindex(i);
      AddChild(item);
    }
#else

    for (unsigned i = 0; i < item_count_; ++i)
    {
      ListViewItem *item = CreateMenuItem(item_type_);
      item->Load(*itemmetric);
      item->LoadFromData(nullptr);
      item->set_dataindex(i);
      items_.push_back(item);
      AddChild(item);
    }

#endif
    items_abs_ = items_;
  }

  // Load sounds
  std::string soundpath;
  if (METRIC->get_safe("Sound.SongSelectChange", soundpath))
    wheel_sound_.Load(soundpath);

  // Build data & item
  RebuildData();
  RebuildItems();

  // selectchange for initiailize.
  if (data_.empty())
    OnSelectChange(nullptr, 0);
  else
    OnSelectChange(data_[data_index_ % size()], 0);

  // Call bar load event
  for (unsigned i = 0; i < items_abs_.size(); ++i)
  {
    std::string cmdname = format_string("Bar%u", i);
    items_abs_[i]->RunCommandByName(cmdname);
  }
}

void ListView::AddData(void* d) { data_.push_back(d); }
void* ListView::GetSelectedMenuData() { return GetMenuDataByIndex(data_index_); }
void* ListView::GetMenuDataByIndex(int index) { return items_[index]; }

void ListView::SelectMenuByIndex(int index)
{
  data_index_ = index;
  data_top_index_ = index - item_center_index_;
  scroll_time_remain_ = 0;
  scroll_delta_ = 0;
  RebuildItems();
}

unsigned ListView::size() const { return (unsigned)data_.size(); }
unsigned ListView::index() const { return data_index_; }

void ListView::Clear()
{
  /* unreference data from bar items. */
  for (auto* p : items_)
    p->LoadFromData(nullptr);
  /* now delete all data. */
  for (auto* p : data_)
    free(p);
  data_.clear();
}

void ListView::set_listviewtype(ListViewType type)
{
  viewtype_ = type;
}

void ListView::set_item_min_index(unsigned min_index)
{
  item_focus_min_ = min_index;
}

void ListView::set_item_max_index(unsigned max_index)
{
  item_focus_max_ = max_index;
}

void ListView::set_item_center_index(unsigned center_idx)
{
  item_center_index_ = center_idx;
}

bool ListView::is_wheel() const
{
  return (viewtype_ == ListViewType::kWheel);
}

void ListView::RebuildData()
{}

void ListView::RebuildItems()
{
  bool loop = is_wheel();
  unsigned data_size = (unsigned)data_.size();
  unsigned data_index = 0;
  unsigned focused_index = data_index_ - data_top_index_;
  unsigned i = 0;
  int loop_count = 0;

  if (data_size > 0)
    data_index = (unsigned)mod_pos(data_top_index_, (int)data_size);

  for (auto *item : items_)
  {
    void* cur_data = nullptr;
    if (!data_.empty())
      cur_data = data_[data_index];

    item->set_dataindex(data_index);
    item->set_focus(focused_index == i);

    if (!loop && loop_count != 0)
    {
      item->Hide();
      //item->LoadFromData(nullptr);
    }
    else
    {
      item->Show();
      if (item->get_data() != cur_data)
        item->LoadFromData(cur_data);
    }

    if (data_size > 0)
      data_index = (data_index + 1) % data_size;

    ++i;
  }
}

void ListView::NavigateDown()
{
  if (!is_wheel() && data_index_ >= (int)size() - 1)
    return;

  // change data index
  data_index_++;
  int item_focus_index = data_index_ - data_top_index_;
  if (item_focus_index > (int)item_focus_max_)
  {
    // back element to front
    std::rotate(items_.begin(), std::prev(items_.end()), items_.end());
    data_top_index_++;

    // start scrolling
    scroll_time_remain_ = scroll_time_;

    // play wheel sound
    wheel_sound_.Play();
  }

  RebuildItems();
  OnSelectChange(
    data_.empty() ? nullptr : data_[mod_pos(data_index_, (int)size())],
    1);
}

void ListView::NavigateUp()
{
  if (!is_wheel() && data_index_ == 0)
    return;

  // change data index
  data_index_--;
  int item_focus_index = data_index_ - data_top_index_;
  if (item_focus_index < (int)item_focus_min_)
  {
    // front element to back
    std::rotate(items_.begin(), std::next(items_.begin()), items_.end());
    data_top_index_--;

    // start scrolling
    scroll_time_remain_ = scroll_time_;

    // play wheel sound
    wheel_sound_.Play();
  }

  RebuildItems();
  OnSelectChange(
    data_.empty() ? nullptr : data_[mod_pos(data_index_, (int)size())],
    -1);
}

void ListView::NavigateLeft() {}

void ListView::NavigateRight() {}

unsigned ListView::CalculateItemCount() const
{
  float height = GetHeight(frame_.pos);
  return static_cast<unsigned>(height / item_height_);
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
  int x, y;
  unsigned item_count = (unsigned)items_.size();

  // Fast-forward animation if they were playing
  for (auto *item : items_)
    item->HurryTween();

  for (unsigned i = 0; i < item_count; ++i)
  {
    // pos: expecting (-1 ~ item_count)
    double pos = i + scroll_delta_;
    x = static_cast<int>(
      pos_expr_param_.curve_size *
      (1.0 - cos(pos / pos_expr_param_.curve_level))
      );
    y = static_cast<int>(
      item_height_ * pos
      );
    items_[i]->SetPos(x, y);
  }
}

void ListView::UpdateItemPosByFixed()
{
  int scroll_offset = (int)floor(scroll_delta_);
  bool loop = is_wheel();
  DrawProperty d;

  // Fast-forward animation if they were playing
  for (auto *item : items_)
    item->HurryTween();

  // actual item array are rotated when listview is scrolled
  // to reduce LoadFromData(..) call, so item object by actual item index
  // should be obtained by items_abs_ array.
  for (unsigned i = 0; i < items_.size() - 1; ++i)
  {
    MakeTween(d,
      items_abs_[i]->get_item_dprop(),
      items_abs_[i+1]->get_item_dprop(),
      scroll_delta_, EaseTypes::kEaseIn);

    // TODO: set alpha, rotation
    items_[i]->SetPos((int)d.pos.x, (int)d.pos.y);
  }
}

void ListView::doUpdate(double delta)
{
  if (scroll_time_remain_ > 0 && scroll_time_ > 0)
  {
    // Update scroll pos
    scroll_time_remain_ = std::max(.0f, scroll_time_remain_ - (float)delta);
    // -- for precision
    if (scroll_time_remain_ < 5.0f)
      scroll_time_remain_ = .0f;
    if (scroll_time_ > 0)
      scroll_delta_ = 1.0f - scroll_time_remain_ / scroll_time_;
  }

  // calculate each bar position-based-index and position
  UpdateItemPos();
}

ListViewItem* ListView::CreateMenuItem(const std::string &)
{
  if (item_type_ == "test")
    return new TestListViewItem();
  else
    return new ListViewItem();
}

void ListView::OnSelectChange(const void *data, int direction) {}

void ListView::OnSelectChanged() {}

}
