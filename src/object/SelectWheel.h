#pragma once

#include "Sprite.h"
#include "Font.h"
#include <string>
#include <memory>

namespace rhythmus
{

constexpr int NUM_SELECT_BAR_TYPES = 10;

enum SelectBarPosMethod
{
  kBarPosExpression,
  kBarPosFixed,
};

/* @brief Actual data which select item uses when rendering */
struct SelectItemData
{
  int type;
  std::string name;
  std::string artist;
};

/* @brief Context of rendering select item */
class SelectItem : public BaseObject
{
public:
  SelectItem();

  SpriteAuto img_sprite_;
  std::unique_ptr<Text> text_sprite_;

  // actual dataindex for this item
  int dataindex;

  // actual barindex (rendering index) for this item
  // 0 indicates center index here.
  int barindex;
};

/* @brief used for calculating bar position of select item
   (with very basic math expression) */
class SelectWheel : public BaseObject
{
public:
  SelectWheel();
  ~SelectWheel();

  void SetCenterIndex(int center_idx);
  void SetCurveLevel(double curve_level);
  void SetBarPosition(int offset, int align = 2 /* 0 left, 1 center, 2 right */);
  void SetBarHeight(int height);
  void SetBarMargin(int margin);
  void SetTextMargin(double x, double y);

  size_t get_select_list_size() const;
  double get_select_bar_scroll() const;
  int get_selected_index() const;
  const char* get_selected_title() const;

  void PushData(const SelectItemData& data);
  void SetSelectBarImage(int type_no, ImageAuto img);
  void SetBarPositionMethod(int use_expr = 1);
  virtual void Prepare(int visible_bar_count);
  virtual void RebuildItems();

  void ScrollDown();
  void ScrollUp();

  /* TODO: curve & rot with z pos? */

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

protected:
  // textures for each bar type
  Sprite* select_bar_src_[NUM_SELECT_BAR_TYPES];

  // select item data
  std::vector<SelectItemData> select_data_;

  // select bars (TODO: use SelectWheelItem)
  std::vector<SelectItem*> bar_;

  // select bar position parameter for kPosTypeExpr
  double curve_level_;
  double curve_size_;
  int bar_height_;
  int bar_margin_;
  int bar_offset_x_;
  int bar_center_y_;
  double text_margin_x_, text_margin_y_;

  // fixed bar position objects used for calculating bar item position.
  // (like LR2 type)
  std::vector<BaseObject*> bar_pos_fixed_;

  // type of calculating select bar position
  // If false, some extern method should update bar position manually.
  int bar_pos_method_;

  // currently focused item index
  int focused_index_;

  // center index of the scroll
  int center_index_;

  // current scroll pos, relative to current focused index.
  double scroll_pos_;

  virtual void SetItemPosByIndex(SelectItem& item, int idx, double r);

  virtual void doUpdate(float delta);

  void UpdateItemPos();
};

}