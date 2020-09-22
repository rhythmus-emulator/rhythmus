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
  dataindex_(0), itemindex_(0),
  data_(nullptr), is_focused_(false), content_(nullptr)
{
}

ListViewItem::ListViewItem(const ListViewItem& obj) : BaseObject(obj),
  dataindex_(obj.dataindex_), itemindex_(obj.itemindex_), data_(obj.data_),
  is_focused_(false), content_(obj.content_), item_dprop_(obj.item_dprop_)
{
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

void ListViewItem::LoadFromLVData(const ListViewData& lvdata)
{
  LoadFromData(lvdata.p);
  margin_ = lvdata.margin;
  dataindex_ = lvdata.index;
}

void ListViewItem::set_dataindex(unsigned dataindex)
{
  dataindex_ = dataindex;
}

unsigned ListViewItem::get_dataindex() const
{
  return dataindex_;
}

void ListViewItem::set_itemindex(int itemindex)
{
  itemindex_ = itemindex;
}

int ListViewItem::get_itemindex() const
{
  return itemindex_;
}

void ListViewItem::set_content(BaseObject *content)
{
  content_ = content;
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

bool ListViewItem::is_empty() const
{
  return data_ == nullptr;
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
  : pos_method_(MenuPosMethod::kMenuPosFixed), index_previous_(-1),
    data_top_index_(0), data_index_(0), viewtype_(ListViewType::kList),
    item_count_(16), set_item_count_auto_(false), item_height_(80), item_total_height_(0),
    item_center_index_(8), item_focus_min_(4), item_focus_max_(12),
    scroll_time_(200.0f), scroll_time_remain_(.0f)
{
  memset(&pos_, 0, sizeof(pos_));
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));
  pos_expr_param_.curve_level = 1.0;
}

ListView::~ListView()
{
  ClearData();
}

void ListView::Load(const MetricGroup &metric)
{
  // Load itemview attributes
  BaseObject::Load(metric);
  metric.get_safe("postype", (int&)pos_method_);
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
  // XXX: repeative item, or pre-create item array?
  // --> that can be decided by setting ...? idk.
  const MetricGroup *itemmetric = metric.get_group("item");
  /* default: nullptr data */
  ListViewItem *item = new ListViewItem();
  if (itemmetric)
    item->Load(*itemmetric);
  item->LoadFromData(nullptr);
  item->set_dataindex(0);
  items_.push_back(item);
  AddChild(item);

  // clone
  // XXX: MAY NOT use clone() method to loading object!
  for (unsigned i = 1; i < item_count_; ++i)
  {
#if 1
    ListViewItem *item = (ListViewItem*)items_[0]->clone();
#else
    ListViewItem *item = CreateMenuItem(item_type_);
    item->Load(*itemmetric);
    item->LoadFromData(nullptr);
#endif
    items_.push_back(item);
    item->set_dataindex(i);
    AddChild(item);
  }

  // Load sounds
  std::string soundpath;
  if (METRIC->get_safe("Sound.SongSelectChange", soundpath))
    wheel_sound_.Load(soundpath);

  // Build data & item
  RebuildData();
  SelectMenuByIndex(0);

  // selectchange for initiailize.
  if (data_.empty())
    OnSelectChange(nullptr, 0);
  else
    OnSelectChange(data_[data_index_ % size()].p, 0);

  // Call listview item load event
  for (unsigned i = 0; i < items_.size(); ++i)
  {
    std::string cmdname = format_string("Bar%u", i);
    items_[i]->RunCommandByName(cmdname);
  }
}

unsigned ListView::size() const { return (unsigned)data_.size(); }
unsigned ListView::index() const { return data_index_; }

bool ListView::is_wheel() const
{
  return (viewtype_ == ListViewType::kWheel);
}

void ListView::AddData(void* d)
{
  data_.push_back(ListViewData{ (unsigned)data_.size(), 0.0f, 0.0f, d, nullptr, Point{0.0f, 0.0f} });
  item_total_height_ += 0;  // TODO
}

void ListView::AddData(void* d, const Point &margin)
{
  data_.push_back(ListViewData{ (unsigned)data_.size(), 0.0f, 0.0f, d, nullptr, margin });
  item_total_height_ += 0;  // TODO
}

void ListView::ClearData()
{
  /* unreference data from bar items. */
  for (auto* p : items_)
    p->LoadFromData(nullptr);
  /* now delete all data. */
  for (auto& data : data_)
  {
    delete data.content;
  }
  data_.clear();
}

void* ListView::GetSelectedMenuData() { return GetMenuDataByIndex(data_index_); }
void* ListView::GetMenuDataByIndex(int index) { return data_[index].p; }

void ListView::SelectMenuByIndex(int index)
{
  data_index_ = index;
  data_top_index_ = index - item_center_index_;
  scroll_time_remain_ = 0;
  RebuildItems();
}

float ListView::GetItemYPosByIndex(int index)
{
  if (data_.empty()) return .0f;
  int dataindex = index % data_.size();
  int loopcount = (index - dataindex) / data_.size();
  return item_total_height_ * loopcount + data_[dataindex].y;
}

void ListView::RebuildData()
{}

void ListView::RebuildItems()
{
  // fill content view to ListViewItem template.
  bool loop = is_wheel();
  unsigned item_index = items_.empty() ? 0 : (unsigned)(
    (pos_.index_i % (int)data_.size() + (int)data_.size()) % (int)data_.size());
  int selindex = pos_.index_i;
  int item_index_raw = pos_.index_i;
  for (auto *item : items_)
  {
    if (data_.empty())
    {
      item->set_content(nullptr);
    }
    else if (!loop && (item_index_raw < 0 || item_index_raw >= items_.size()))
    {
      item->Hide();
    }
    else if (item->is_empty() || item->get_dataindex() != item_index)
    {
      ListViewData &data = data_[item_index];
      RebuildDataContent(data);
      item->LoadFromLVData(data);
      item->set_itemindex(selindex);
      item->set_focus(data_index_ == item_index_raw);
      item->Show();
    }
    item_index_raw++;
    selindex++;
    item_index = items_.empty() ? 0 : (item_index + 1) % data_.size();
  }
}

void ListView::RebuildDataContent(ListViewData &data)
{}

ListViewItem *ListView::GetLVItem(unsigned data_index)
{
  R_ASSERT(data_index < items_.size());
  return items_[data_index];
}

BaseObject *ListView::CreateLVItemContent(void *data)
{
  /* Implement by your own */
  return nullptr;
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

void ListView::NavigateDown()
{
  if (!is_wheel() && data_index_ >= (int)size() - 1)
    return;

  // change data index
  data_index_++;
  int item_focus_index = data_index_ - data_top_index_;
  if (item_focus_index > (int)item_focus_max_)
  {
    // do scolling
    scroll_idx_from_ = data_top_index_;
    scroll_idx_to_ = data_top_index_ + 1;
    scroll_time_remain_ = scroll_time_;
    data_top_index_++;

    // play wheel sound
    wheel_sound_.Play();
  }

  RebuildItems();
  OnSelectChange(
    data_.empty() ? nullptr : data_[mod_pos(data_index_, (int)size())].p,
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
    // do scolling
    scroll_idx_from_ = data_top_index_;
    scroll_idx_to_ = data_top_index_ - 1;
    scroll_time_remain_ = scroll_time_;
    data_top_index_--;

    // play wheel sound
    wheel_sound_.Play();
  }

  RebuildItems();
  OnSelectChange(
    data_.empty() ? nullptr : data_[mod_pos(data_index_, (int)size())].p,
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
  DrawProperty d;
  float ratio = fmod(pos_.index_f, 1);
  unsigned j = 0;
  int x, y;

#if 0
  // Fast-forward animation if they were playing
  for (auto *item : items_)
    item->HurryTween();

  // XXX: Update motion to item_pos_list if they were in tweening
#endif

  switch (pos_method_)
  {
  case MenuPosMethod::kMenuPosFixed:
    // XXX: copy to item_pos_list_ from items_->get_item_dprop() ?
    // actual item array are rotated when listview is scrolled
    // to reduce LoadFromData(..) call, so item object by actual item index
    // should be obtained by items_abs_ array.
    for (unsigned i = 0; i + 1 < items_.size(); ++i)
    {
      MakeTween(d,
        items_[i]->get_item_dprop(),
        items_[i + 1]->get_item_dprop(),
        ratio, EaseTypes::kEaseIn);

      // TODO: set alpha, rotation
      items_[i]->SetPos((int)d.pos.x, (int)d.pos.y);
    }
    break;
  case MenuPosMethod::kMenuPosDynamic:
    for (unsigned i = 0; i + 1 < items_.size(); ++i)
    {
      float ypos = GetItemYPosByIndex(items_[i]->get_itemindex()) - pos_.y;
      float dist = 0;
      for (; j + 1 < item_pos_list_.size(); ++j)
      {
        if (ypos < item_pos_list_[j + 1].y)
          break;
      }
      dist = item_pos_list_[j + 1].y - item_pos_list_[j].y;

      MakeTween(d,
        item_pos_list_[j].p,
        item_pos_list_[j + 1].p,
        (ypos - item_pos_list_[j].y) / dist, EaseTypes::kEaseIn);

      // TODO: set alpha, rotation
      items_[i]->SetPos((int)d.pos.x, (int)d.pos.y);
    }
    break;
  case MenuPosMethod::kMenuPosExpr:
    for (unsigned i = 0; i < items_.size(); ++i)
    {
      // pos: expecting (-1 ~ item_count)
      float pos = i + ratio;
      x = static_cast<int>(
        pos_expr_param_.curve_size *
        (1.0 - cos(pos / pos_expr_param_.curve_level))
        );
      y = static_cast<int>(
        item_height_ * pos
        );
      items_[i]->SetPos(x, y);
    }
    break;
  }
}

void ListView::doUpdate(double delta)
{
  // Update current scroll and index
  if (scroll_time_remain_ > 0 && scroll_time_ > 0)
  {
    // current scroll delta ( 0 ~ 1 )
    float scroll_ratio;

    // Update scroll pos
    scroll_time_remain_ = std::max(.0f, scroll_time_remain_ - (float)delta);
    // -- for precision
    if (scroll_time_remain_ < 5.0f)
      scroll_time_remain_ = .0f;
    if (scroll_time_ > 0)
      scroll_ratio = 1.0f - scroll_time_remain_ / scroll_time_;

    pos_.index_f = scroll_idx_from_ * (1.0f - scroll_ratio) + scroll_idx_to_ * scroll_ratio;
    pos_.index_i = (int)pos_.index_f;
    // TODO: multiply listviewitem total height if scroll_idx_to is out of index.
    pos_.y = GetItemYPosByIndex(scroll_idx_from_) * (1.0f - scroll_ratio)
           + GetItemYPosByIndex(scroll_idx_to_) * scroll_ratio;
  }

  // calculate each LVitem position-based-index and position
  UpdateItemPos();

  // rebuild LVitem if current index is changed
  if (index_current_ != pos_.index_i)
  {
    RebuildItems();
    index_current_ = pos_.index_i;
  }
}

void ListView::OnSelectChange(const void *data, int direction) {}

void ListView::OnSelectChanged() {}

}
