#include "Wheel.h"
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

WheelItem::WheelItem() :
  dataindex_(0), itemindex_(0),
  data_(nullptr), is_focused_(false), content_(nullptr)
{
  memset(&item_dprop_.pos.a, 0, sizeof(Vector4));
  item_dprop_.align.x = item_dprop_.align.y = 0.5f;
  memset(&item_dprop_.rotate.x, 0, sizeof(Vector3));
  item_dprop_.scale.x = item_dprop_.scale.y = 1.0f;
  item_dprop_.color.a = item_dprop_.color.r = item_dprop_.color.g = item_dprop_.color.b = 1.0f;
}

WheelItem::WheelItem(const WheelItem& obj) : BaseObject(obj),
  dataindex_(obj.dataindex_), itemindex_(obj.itemindex_), data_(obj.data_),
  is_focused_(false), content_(obj.content_), item_dprop_(obj.item_dprop_)
{
}

BaseObject *WheelItem::clone()
{
  return new WheelItem(*this);
}

void WheelItem::Load(const MetricGroup &m)
{
  BaseObject::Load(m);

  item_dprop_ = frame_;
}

void WheelItem::LoadFromData(void *data)
{
  data_ = data;
}

void WheelItem::LoadFromWheelData(const WheelItemData& lvdata)
{
  LoadFromData(lvdata.p);
  margin_ = lvdata.margin;
  dataindex_ = lvdata.index;
}

void WheelItem::set_dataindex(unsigned dataindex)
{
  dataindex_ = dataindex;
}

unsigned WheelItem::get_dataindex() const
{
  return dataindex_;
}

void WheelItem::set_itemindex(int itemindex)
{
  itemindex_ = itemindex;
}

int WheelItem::get_itemindex() const
{
  return itemindex_;
}

void WheelItem::set_content(BaseObject *content)
{
  content_ = content;
}

void WheelItem::set_focus(bool focused)
{
  is_focused_ = focused;
}

void* WheelItem::get_data()
{
  return data_;
}

DrawProperty &WheelItem::get_item_dprop()
{
  return item_dprop_;
}

bool WheelItem::is_empty() const
{
  return data_ == nullptr;
}

void WheelItem::OnAnimation(DrawProperty &frame)
{
  item_dprop_ = frame;
}

// @brief Test listviewitem for test purpose
class TestListViewItem : public WheelItem
{
public:
private:
};

// --------------------------------- class Menu

Wheel::Wheel()
  : pos_method_(WheelPosMethod::kMenuPosExpr), is_loop_(true), index_previous_(-1),
    data_top_index_(0), data_index_(0),
    item_count_(16), set_item_count_auto_(false), item_height_(80), item_total_height_(0),
    item_center_index_(8), item_focus_min_(4), item_focus_max_(12),
    scroll_time_(200.0f), scroll_time_remain_(.0f)
{
  memset(&pos_, 0, sizeof(pos_));
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));
  pos_expr_param_.curve_level = 1.0;
}

Wheel::~Wheel()
{
  ClearData();
}

void Wheel::Load(const MetricGroup &metric)
{
  unsigned wheelwrappersize = 30;

  // Load itemview attributes
  BaseObject::Load(metric);
  metric.get_safe("postype", (int&)pos_method_);
  metric.get_safe("itemtype", item_type_);
  metric.get_safe("itemheight", item_height_);
  metric.get_safe("itemcountauto", set_item_count_auto_);
  metric.get_safe("itemcount", (int&)wheelwrappersize);
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

  SetWheelWrapperCount(wheelwrappersize);
  const MetricGroup *itemmetric = metric.get_group("item");
  if (itemmetric) {
    for (auto *item : items_)
      item->Load(*itemmetric);
  }
}

void Wheel::OnReady()
{
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

unsigned Wheel::size() const { return (unsigned)data_.size(); }
unsigned Wheel::index() const { return data_index_; }

void Wheel::AddData(void* d)
{
  data_.push_back(WheelItemData{ (unsigned)data_.size(), 0.0f, 0.0f, d, nullptr, Point{0.0f, 0.0f} });
  item_total_height_ += 0;  // TODO
}

void Wheel::AddData(void* d, const Point &margin)
{
  data_.push_back(WheelItemData{ (unsigned)data_.size(), 0.0f, 0.0f, d, nullptr, margin });
  item_total_height_ += 0;  // TODO
}

void Wheel::ClearData()
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

void* Wheel::GetSelectedMenuData() { return GetMenuDataByIndex(data_index_); }
void* Wheel::GetMenuDataByIndex(unsigned index) { return data_[index].p; }
void* Wheel::GetMenuItemWrapperByIndex(unsigned index) { return items_[index]; }
unsigned Wheel::GetMenuItemWrapperCount() const { return items_.size(); };

void Wheel::SelectMenuByIndex(int index)
{
  data_index_ = index;
  data_top_index_ = index - item_center_index_;
  scroll_time_remain_ = 0;
  RebuildItems();
}

float Wheel::GetItemYPosByIndex(int index)
{
  if (data_.empty()) return .0f;
  int dataindex = index % data_.size();
  int loopcount = (index - dataindex) / data_.size();
  return item_total_height_ * loopcount + data_[dataindex].y;
}

void Wheel::RebuildData()
{}

void Wheel::RebuildItems()
{
  // fill content view to ListViewItem template.
  bool loop = is_loop_;
  unsigned item_index = 0;
  int selindex = pos_.index_i;
  int item_index_raw = pos_.index_i;

  if (data_.size() == 0) return;

  item_index = items_.empty() ? 0 : (unsigned)(
    (data_top_index_ % (int)data_.size() + (int)data_.size()) % (int)data_.size());

  for (auto *item : items_)
  {
    if (data_.empty())
    {
      // set empty content (TODO: fix this method)
      item->set_content(nullptr);
    }
    else if (!loop && (item_index_raw < 0 || item_index_raw >= items_.size()))
    {
      // hide looping item
      item->Hide();
    }
    else if (item->is_empty() || item->get_dataindex() != item_index)
    {
      WheelItemData &data = data_[item_index];
      RebuildDataContent(data);
      item->LoadFromWheelData(data);
      item->set_itemindex(selindex);
      item->set_focus(data_index_ == item_index_raw);
      item->Show();
    }
    item_index_raw++;
    selindex++;
    item_index = items_.empty() ? 0 : (item_index + 1) % data_.size();
  }
}

void Wheel::RebuildDataContent(WheelItemData &data)
{}

WheelItem *Wheel::GetWheelItem(unsigned data_index)
{
  R_ASSERT(data_index < items_.size());
  return items_[data_index];
}

BaseObject *Wheel::CreateLVItemContent(void *data)
{
  /* Implement by your own */
  return nullptr;
}

WheelItem *Wheel::CreateWheelWrapper()
{
  /* Implement by your own */
  return nullptr;
}

void Wheel::SetWheelWrapperCount(unsigned max_size)
{
  if (max_size < items_.size()) {
    for (unsigned i = max_size; i < items_.size(); ++i)
    {
      RemoveChild(items_[i]);
      delete items_[i];
    }
    items_.resize(max_size);
  }
  else if (max_size > items_.size()) {
    while (items_.size() < max_size) {
      auto *item = CreateWheelWrapper();
      items_.push_back(item);
      AddChild(item);
    }
  }
}

void Wheel::SetWheelWrapperCount(unsigned max_size, const std::string &item_type)
{
  if (item_type_ == item_type) {
    SetWheelWrapperCount(max_size);
  }
  else {
    // re-create all wrappers.
    SetWheelWrapperCount(0);
    item_type_ = item_type;
    SetWheelWrapperCount(max_size);
  }
}

void Wheel::SetWheelPosMethod(WheelPosMethod method)
{
  pos_method_ = method;
}

void Wheel::SetWheelItemType(const std::string &item_type)
{
  unsigned itemcount = 0;
  if (item_type_ == item_type) return;
  item_type_ = item_type;

  // re-create all wrappers.
  itemcount = items_.size();
  SetWheelWrapperCount(0);
  SetWheelWrapperCount(itemcount);
}

void Wheel::set_item_min_index(unsigned min_index)
{
  item_focus_min_ = min_index;
}

void Wheel::set_item_max_index(unsigned max_index)
{
  item_focus_max_ = max_index;
}

void Wheel::set_item_center_index(unsigned center_idx)
{
  item_center_index_ = center_idx;
}

void Wheel::NavigateDown()
{
  if (!is_loop_ && data_index_ >= (int)size() - 1)
    return;

  // finish animation if not
  for (auto *i : items_)
    i->HurryTween();

  // change data index
  data_index_++;
  int item_focus_index = data_index_ - data_top_index_;

  // do scolling only if focus index is out of setting.
  if (item_focus_index > (int)item_focus_max_)
  {
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

void Wheel::NavigateUp()
{
  if (!is_loop_ && data_index_ == 0)
    return;

  // finish animation if not
  for (auto *i : items_)
    i->HurryTween();

  // change data index
  data_index_--;
  int item_focus_index = data_index_ - data_top_index_;

  // do scolling only if focus index is out of setting.
  if (item_focus_index < (int)item_focus_min_)
  {
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

void Wheel::NavigateLeft() {}

void Wheel::NavigateRight() {}

unsigned Wheel::CalculateItemCount() const
{
  float height = rhythmus::GetHeight(frame_.pos);
  return static_cast<unsigned>(height / item_height_);
}

void Wheel::UpdateItemPos()
{
  DrawProperty d;
  float ratio =
    (float)(scroll_idx_from_ > scroll_idx_to_ ? scroll_idx_from_ : scroll_idx_to_)
    - pos_.index_f;
  unsigned j = 0;
  int x, y;

  /* when from > to (NavigateUp),
   * then ratio change from 0.0f to 1.0f.
   * when from < to (NavigateDown), 
   * ratio change from 1.0f to 0.0f. */

  switch (pos_method_)
  {
  case WheelPosMethod::kMenuPosFixed:
    // XXX: copy to item_pos_list_ from items_->get_item_dprop() ?
    // actual item array are rotated when listview is scrolled
    // to reduce LoadFromData(..) call, so item object by actual item index
    // should be obtained by items_abs_ array.
    for (unsigned i = 1; i + 1 < items_.size(); ++i) {
      if (scroll_idx_from_ > scroll_idx_to_) {
        MakeTween(d,
          items_[i]->get_item_dprop(),
          items_[i + 1]->get_item_dprop(),
          1.0f - ratio, EaseTypes::kEaseIn);
      }
      else {
        MakeTween(d,
          items_[i]->get_item_dprop(),
          items_[i - 1]->get_item_dprop(),
          ratio, EaseTypes::kEaseIn);
      }

      // TODO: set alpha, rotation
      items_[i]->SetPos((int)d.pos.x, (int)d.pos.y);
    }
    break;
  case WheelPosMethod::kMenuPosExpr:
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

void Wheel::doUpdate(double delta)
{
  // Update current scroll and index position
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

    // calculate each LVitem position-based-index and position
    UpdateItemPos();
  }

  // rebuild LVitem if current index is changed
  if (index_current_ != pos_.index_i)
  {
    RebuildItems();
    index_current_ = pos_.index_i;
  }
}

void Wheel::OnSelectChange(const void *data, int direction) {}

void Wheel::OnSelectChanged() {}

}
