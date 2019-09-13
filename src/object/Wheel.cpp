#include "Wheel.h"
#include "Game.h"

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

WheelItem::WheelItem(int barindex)
  : barindex_(barindex), dataindex_(0),
    data_(nullptr), is_selected_(false)
{
}

void WheelItem::Invalidate()
{
}

void WheelItem::set_data(int dataidx, void* d)
{
  dataindex_ = dataidx;
  data_ = d;
}

int WheelItem::get_dataindex() const { return dataindex_; }

void* WheelItem::get_data() { return data_; }

int WheelItem::get_barindex() const { return barindex_; }

void WheelItem::set_is_selected(int current_index)
{
  is_selected_ = (current_index == barindex_);
}

// -------------------------------- class Wheel

Wheel::Wheel()
  : focused_index_(0), center_index_(0), scroll_pos_(0), inf_scroll_(true),
    pos_method_(WheelItemPosMethod::kBarPosExpression)
{
  memset(&pos_expr_param_, 0, sizeof(pos_expr_param_));

  // Set bar position expr param with default
  pos_expr_param_.bar_offset_x = Game::getInstance().get_window_width() - 540;
  pos_expr_param_.bar_center_y = Game::getInstance().get_window_height() / 2;
  pos_expr_param_.bar_height = 40;
  pos_expr_param_.bar_width = 540;
  pos_expr_param_.bar_margin = 1;
  pos_expr_param_.curve_size = 80;
  pos_expr_param_.curve_level = 5;

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

  // XXX: temp
  wheel_sound_.Load("../sound/SelfEvolution/scratch.wav");
}

Wheel::~Wheel()
{
  Clear();
}

void Wheel::SetCenterIndex(int center_idx)
{
  center_index_ = center_idx;
}

void Wheel::Clear()
{
  // same as releasing all wheelitem objects
  Prepare(0);
  for (void* p : select_data_)
    free(p);
  select_data_.clear();
}

size_t Wheel::get_data_size() const
{
  return select_data_.size();
}

/* Get selection scroll. */
double Wheel::get_select_bar_scroll() const
{
  return scroll_pos_;
}

int Wheel::get_selected_index() const
{
  return focused_index_;
}

/* should consider center index */
WheelItem* Wheel::get_item(int index)
{
  return bar_[(index + center_index_) % bar_.size()];
}

int Wheel::get_selected_dataindex() const
{
  /* bar 0 index : center */
  return bar_[0]->get_dataindex();
}

void Wheel::AddData(void* data)
{
  select_data_.push_back(data);
}

void Wheel::Prepare(int visible_bar_count)
{
  for (auto *obj : bar_)
  {
    RemoveChild(obj);
    delete obj;
  }
  bar_.clear();

  for (int i = 0; i < visible_bar_count; ++i)
  {
    auto *o = CreateWheelItem(i);
    o->set_parent(this);
    bar_.push_back(o);
    AddChild(bar_.back());
  }
}

void Wheel::RebuildItems()
{
  /* Won't rebuild (or build default null item) */
  if (get_data_size() == 0)
    return;

  size_t disp_cnt = bar_.size();
  int last_bar_idx = disp_cnt - center_index_;

  for (auto i = 0; i < disp_cnt; ++i)
  {
    //int ii = (i + focused_index_ + center_index_) % disp_cnt;
    int dataindex;
    if (i < last_bar_idx)
      dataindex = mod_pos(focused_index_ + i, select_data_.size());
    else
      dataindex = mod_pos(focused_index_ + i - disp_cnt, select_data_.size());
    auto& data = select_data_[dataindex];
    auto& bar = *bar_[i]; /* XXX: 0 index bar is center bar! */
    bar.set_data(dataindex, data);
    bar.Invalidate();
  }
}

void Wheel::set_infinite_scroll(bool inf_scroll)
{
  inf_scroll_ = inf_scroll;
}

void Wheel::ScrollDown()
{
  if (!inf_scroll_ && focused_index_ + 1 >= select_data_.size())
    return;

  focused_index_ = (focused_index_ + 1) % select_data_.size();
  scroll_pos_ -= 0.9999;  // a trick to avoid zero by mod 1
  if (scroll_pos_ < -kScrollPosMaxDiff)
    scroll_pos_ = -kScrollPosMaxDiff;
  RebuildItems();
  UpdateItemPos();
  wheel_sound_.Play();
}

void Wheel::ScrollUp()
{
  if (!inf_scroll_ && focused_index_ < 1)
    return;

  focused_index_--;
  if (focused_index_ < 0) focused_index_ += select_data_.size();
  scroll_pos_ += 0.9999;  // a trick to avoid zero by mod 1
  if (scroll_pos_ > kScrollPosMaxDiff)
    scroll_pos_ = kScrollPosMaxDiff;
  RebuildItems();
  UpdateItemPos();
  wheel_sound_.Play();
}

void Wheel::doUpdate(float delta)
{
  // Update scroll pos
  if ((scroll_pos_ < 0 && scroll_pos_ > -0.05) ||
      (scroll_pos_ > 0 && scroll_pos_ < 0.05))
    scroll_pos_ = 0;
  else
    scroll_pos_ -= scroll_pos_ * 0.3;

  // calculate each bar position-based-index and position
  UpdateItemPos();
}

void Wheel::UpdateItemPos()
{
  switch (pos_method_)
  {
  case WheelItemPosMethod::kBarPosExpression:
    UpdateItemPosByExpr();
    break;
  case WheelItemPosMethod::kBarPosFixed:
    UpdateItemPosByFixed();
    break;
  }
}

void Wheel::UpdateItemPosByExpr()
{
  float pos = scroll_pos_;
  int disp_cnt = (int)bar_.size();
  int x, y;

  for (int i = 0; i < disp_cnt; ++i)
  {
    // break if index is out of data and not infinite scrolling
    if (!inf_scroll_ &&
      i >= (int)select_data_.size() - focused_index_ &&
      i < (int)bar_.size() - focused_index_)
    {
      bar_[i]->Hide();
      continue;
    }
    if (!inf_scroll_ && ((i < center_index_ && i + focused_index_ >= select_data_.size()) ||
      (i >= center_index_ && (int)bar_.size() - i > focused_index_)))
    {
      bar_[i]->Hide();
      continue;
    }


    int ii = i < disp_cnt - center_index_ ?
      i : i - disp_cnt;
    x = pos_expr_param_.bar_offset_x + pos_expr_param_.curve_size *
      (1.0 - cos((double)(ii - pos) / pos_expr_param_.curve_level));
    y = pos_expr_param_.bar_center_y +
      (pos_expr_param_.bar_height + pos_expr_param_.bar_margin) * (ii - pos) -
      pos_expr_param_.bar_height * 0.5f;
    bar_[i]->SetPos(x, y);
    bar_[i]->SetSize(pos_expr_param_.bar_width, pos_expr_param_.bar_height);
    bar_[i]->Show();
  }
}

void Wheel::UpdateItemPosByFixed()
{
  float r = scroll_pos_; //fmod(pos, 1.0f);

  // XXX: get correct draw property with scroll_pos bigger than 1

  for (int i = 0; i < bar_.size(); ++i)
  {
    // break if index is out of data and not infinite scrolling
    if (!inf_scroll_ &&
      i >= (int)select_data_.size() - focused_index_ &&
      i < (int)bar_.size() - focused_index_)
    {
      bar_[i]->Hide();
      continue;
    }
    if (!inf_scroll_ && ((i < center_index_ && i + focused_index_ >= select_data_.size()) ||
      (i >= center_index_ && (int)bar_.size() - i > focused_index_)))
    {
      bar_[i]->Hide();
      continue;
    }

    // get tween index used for current bar.
    int ii = (i + center_index_) % bar_.size() - 1 /* LR2 compat. */;
    float ri = r;
    if (r < 0) {
      ii++;
      ri += 1.0f; // XXX: bad code
    }
    if (ii < 0 || ii > kDefaultBarCount - 1)
    {
      if (ii >= 0)
        bar_[ii]->Hide();
      continue;
    }

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
      ri, EaseTypes::kEaseLinear);
    bar_[i]->Show();
    bar_[i]->LoadDrawProperty(d);
  }
}

void Wheel::doRender()
{
}

void Wheel::LoadProperty(const std::string& prop_name, const std::string& value)
{
  /* LR2 type commands */
  if (prop_name == "#DST_BAR_BODY_ON")
  {
    if (bar_.empty()) Prepare(kDefaultBarCount);

    pos_method_ =
      WheelItemPosMethod::kBarPosFixed;

    int attr = atoi(GetFirstParam(value).c_str());
    if (attr < 0 || attr >= kDefaultBarCount) return;
    pos_fixed_param_.tween_bar_focus[attr].LoadProperty(prop_name, value);
  }
  else if (prop_name == "#DST_BAR_BODY_OFF")
  {
    // if not initialized, then do it
    if (bar_.empty()) Prepare(kDefaultBarCount);

    // set pos type automatically
    pos_method_ =
      WheelItemPosMethod::kBarPosFixed;

    int attr = atoi(GetFirstParam(value).c_str());
    if (attr < 0 || attr > kDefaultBarCount) return;
    pos_fixed_param_.tween_bar[attr].LoadProperty(prop_name, value);
  }
  else if (prop_name == "#SRC_BAR_BODY")
  {
    int attr = atoi(GetFirstParam(value).c_str());
    if (attr < 0 || attr >= 30) return;

    Sprite *spr = new Sprite();
    spr->set_name("BARIMG" + std::to_string(attr));
    RegisterChild(spr);
    AddChild(spr);

    // not for rendering but for updating
    spr->Hide();

    spr->LoadProperty(prop_name, value);
  }
  else if (prop_name == "#BAR_CENTER")
  {
    center_index_ = atoi(GetFirstParam(value).c_str());
  }
}

WheelItem* Wheel::CreateWheelItem(int index)
{
  return new WheelItem(index);
}

}