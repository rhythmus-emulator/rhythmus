#include "Menu.h"
#include "Game.h"
#include "SceneManager.h"
#include "Util.h"

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

// ---------------------------- class WheelItem

MenuItem::MenuItem()
  : barindex_(0), data_(nullptr), is_focused_(false)
{
}

void MenuItem::LoadFromMenuData(MenuData *d)
{
  data_ = d;
}

void MenuItem::set_focus(bool focused)
{
  is_focused_ = focused;
}

MenuData* MenuItem::get_data()
{
  return data_;
}

// --------------------------------- class Menu

Menu::Menu()
  : data_index_(0),
    display_count_(16), focus_index_(8), focus_min_(4), focus_max_(12),
    scroll_delta_(0), inf_scroll_(false), index_delta_(0),
    pos_method_(MenuPosMethod::kMenuPosExpression)
{
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));

  // Set bar position expr param with default
  pos_expr_param_.bar_offset_x = Graphic::getInstance().width() - 540;
  pos_expr_param_.bar_center_y = Graphic::getInstance().height() / 2;
  pos_expr_param_.bar_height = 40;
  pos_expr_param_.bar_width = 540;
  pos_expr_param_.bar_margin = 1;
  pos_expr_param_.curve_size = 80;
  pos_expr_param_.curve_level = 5;


  /**
   * Preserved names:
   * (Bar sprite for type n)    BARIMG1 ~ BARIMGn
   * (Position for not focused) BARPOS1 ~ BARPOS30
   * (Position for focused)     BARPOSACTIVE1 ~ BARPOSACTIVE30
   * --> used when MenuPosMethod is Fixed.
   */

  // Add all tween_bar objects for tween updating & searching
  for (int i = 0; i < kDefaultBarCount + 1; ++i)
  {
    pos_fixed_param_.tween_bar[i].set_name("BARPOS" + std::to_string(i));
    pos_fixed_param_.tween_bar_focus[i].set_name("BARPOSACTIVE" + std::to_string(i));
    pos_fixed_param_.tween_bar[i].Hide();
    pos_fixed_param_.tween_bar_focus[i].Hide();
    AddChild(&pos_fixed_param_.tween_bar[i]);
    AddChild(&pos_fixed_param_.tween_bar_focus[i]);
  }
}

Menu::~Menu()
{
  Clear();
}

void Menu::AddData(MenuData* d) { data_.push_back(d); }
MenuData& Menu::GetSelectedMenuData() { return GetMenuDataByIndex(data_index_); }
MenuData& Menu::GetMenuDataByIndex(int index) { return *data_[index]; }

MenuData* Menu::GetMenuDataByName(const std::string& name)
{
  if (name.empty())
    return nullptr;
  for (auto *d : data_)
  {
    if (d->name == name)
      return d;
  }
  return nullptr;
}

void Menu::SelectMenuByName(const std::string& name)
{
  for (int i = 0; i < data_.size(); ++i) if (data_[i]->name == name)
  {
    SelectMenuByIndex(i);
    return;
  }

  // -- failed to select menu by name --
}

void Menu::SelectMenuByIndex(int index)
{
  data_index_ = index;
  RebuildItems();
}

int Menu::size() const { return data_.size(); }
int Menu::index() const { return data_index_; }

void Menu::Clear()
{
  for (auto* p : bar_)
  {
    RemoveChild(p);
    delete p;
  }
  for (auto* p : data_)
    delete p;
  bar_.clear();
  data_.clear();
}

void Menu::set_display_count(int display_count)
{
  display_count_ = display_count;
}

void Menu::set_focus_min_index(int min_index)
{
  focus_min_ = min_index;
}

void Menu::set_focus_max_index(int max_index)
{
  focus_max_ = max_index;
}

void Menu::set_focus_index(int center_idx)
{
  focus_index_ = center_idx;
}

void Menu::set_infinite_scroll(bool inf_scroll)
{
  inf_scroll_ = inf_scroll;
}

void Menu::RebuildItems()
{
  int start_data_idx = data_index_ - focus_index_ - kScrollPosMaxDiff;
  int end_data_idx = start_data_idx + display_count_ + kScrollPosMaxDiff * 2;
  int data_idx = start_data_idx;
  int build_item_count = display_count_ + kScrollPosMaxDiff * 2;

  /**
   * Before reset item, shift it.
   * (This will induce item invalidation less times)
   */
  // TODO
  index_delta_ = 0;

  /**
   * Reset items
   * @warn if not looping, then don't generate item over index, exit.
   */
  for (auto *item : bar_)
  {
    int data_idx_prev = data_idx;
    int data_idx_safe = mod_pos(data_idx, size());
    data_idx++;
    if (!inf_scroll_ && (data_idx_prev < 0 || data_idx_prev >= size()))
    {
      item->Hide();
      continue;
    }

    MenuData* cur_data = data_[data_idx_safe];
    if (item->get_data() == cur_data)
      continue; // no need to invalidate

    item->LoadFromMenuData(cur_data);
  }
}

void Menu::NavigateDown()
{
  if (!inf_scroll_ && data_index_ >= size() - 1)
    return;

  wheel_sound_.Play();

  data_index_ = (data_index_ + 1) % size();
  if (focus_index_ < focus_max_)
  {
    // As only focus changed, no need to rebuild item.
    // just change item focus and exit
    bar_[focus_index_]->set_focus(false);
    focus_index_++;
    bar_[focus_index_]->set_focus(true);
    return;
  }

  // below here will cause wheel-rotating effect

  scroll_delta_ -= 1.0;
  if (scroll_delta_ < -kScrollPosMaxDiff)
    scroll_delta_ = -kScrollPosMaxDiff;

  RebuildItems();
  UpdateItemPos();
}

void Menu::NavigateUp()
{
  if (!inf_scroll_ && data_index_ == 0)
    return;

  wheel_sound_.Play();

  data_index_--;
  if (data_index_ < 0) data_index_ += size();
  if (focus_index_ > focus_min_)
  {
    // As only focus changed, no need to rebuild item.
    // just change item focus and exit
    bar_[focus_index_]->set_focus(false);
    focus_index_--;
    bar_[focus_index_]->set_focus(true);
    return;
  }

  // below here will cause wheel-rotating effect

  scroll_delta_ += 1.0;
  if (scroll_delta_ > kScrollPosMaxDiff)
    scroll_delta_ = kScrollPosMaxDiff;

  RebuildItems();
  UpdateItemPos();
}

void Menu::NavigateLeft() {}

void Menu::NavigateRight() {}

void Menu::doUpdate(float delta)
{
  // Update scroll pos
  if ((scroll_delta_ < 0 && scroll_delta_ > -0.05) ||
      (scroll_delta_ > 0 && scroll_delta_ < 0.05))
    scroll_delta_ = 0;
  else
    scroll_delta_ -= scroll_delta_ * 0.3;

  // calculate each bar position-based-index and position
  UpdateItemPos();
}

void Menu::UpdateItemPos()
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

void Menu::UpdateItemPosByExpr()
{
  int x, y, i;

  // firstly - set all object's position
  for (i = 0; i < display_count_ + kScrollPosMaxDiff * 2; ++i)
  {
    double pos = i - display_count_ / 2 + scroll_delta_;
    x = pos_expr_param_.bar_offset_x + pos_expr_param_.curve_size *
      (1.0 - cos(pos / pos_expr_param_.curve_level));
    y = pos_expr_param_.bar_center_y +
      (pos_expr_param_.bar_height + pos_expr_param_.bar_margin) * pos -
      pos_expr_param_.bar_height * 0.5f;
    bar_[i]->SetPos(x, y);
    bar_[i]->SetSize(pos_expr_param_.bar_width, pos_expr_param_.bar_height);
  }

  // decide range of object to show
  int start_idx_show = kScrollPosMaxDiff + floor(scroll_delta_);
  int end_idx_show = start_idx_show + display_count_;
  for (i = 0; i < display_count_ + kScrollPosMaxDiff * 2; ++i)
  {
    if (i < start_idx_show || i > end_idx_show)
      bar_[i]->Hide();
    else
      bar_[i]->Show();
  }
}

void Menu::UpdateItemPosByFixed()
{
  int i, ii;
  double ratio = scroll_delta_ - floor(scroll_delta_);

  // decide range of object to show
  int start_idx_show = kScrollPosMaxDiff + floor(scroll_delta_) + 1 /* LR2 specific trick */;
  int end_idx_show = start_idx_show +
    display_count_ > 28 ? 28 : display_count_ /* XXX: Number is limited here! */;
  for (i = 0; i < display_count_ + kScrollPosMaxDiff * 2; ++i)
  {
    if (i < start_idx_show || i >= end_idx_show)
      bar_[i]->Hide();
    else
    {
      ii = i - start_idx_show;
      ASSERT(ii >= 0 && ii < 30);
      BaseObject *obj1 = &pos_fixed_param_.tween_bar[ii + 1];
      BaseObject *obj2 = &pos_fixed_param_.tween_bar[ii];
      if (!obj1->get_draw_property().display || !obj2->get_draw_property().display)
      {
        bar_[i]->Hide();
        continue;
      }

      DrawProperty d;
      MakeTween(d,
        obj1->get_draw_property(),
        obj2->get_draw_property(),
        ratio, EaseTypes::kEaseLinear);
      // TODO: apply alpha?
      bar_[i]->SetPos(d.x, d.y);
    }
  }
}

void Menu::doRender()
{
}

MenuItem* Menu::CreateMenuItem()
{
  return new MenuItem();
}

void Menu::Load(const Metric &metric)
{
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
  while (bar_.size() < display_count_)
  {
    auto *item = CreateMenuItem();
    item->set_parent(this);
    item->Load(metric);
  }

  // Build items (position)
  for (int i = 0; i < display_count_; ++i)
  {
    // TODO: need to make new metric for this bar?
    // Bar1OnLoad --> OnLoad ?
    pos_fixed_param_.tween_bar[i].Load(metric);
  }

  // Load sounds
  wheel_sound_.Load(
    Setting::GetThemeMetricList().get<std::string>("Sound", "MusicSelect")
  );

  // Build item and set basic position
  RebuildItems();
  UpdateItemPos();
}

}