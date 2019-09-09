#pragma once

#include "Sprite.h"
#include "Font.h"
#include "Sound.h"
#include <string>
#include <memory>

namespace rhythmus
{

constexpr int NUM_SELECT_BAR_TYPES = 10;
constexpr int kDefaultBarCount = 30;
constexpr int kScrollPosMaxDiff = 1;

enum WheelItemPosMethod
{
  kBarPosExpression,
  kBarPosFixed,
};

/* @brief Context of rendering select item */
class WheelItem : public Sprite
{
public:
  WheelItem(int barindex);

  /**
   * @brief
   * Invalidate current item by using own property.
   * Call this method when something rendering
   * context is changed.
   * e.g. Change of song difficulty
   */
  virtual void Invalidate();

  void set_data(int dataidx, void* d);

  int get_dataindex() const;

  void* get_data();

  int get_barindex() const;

  void set_is_selected(int current_index);

private:
  // actual barindex (rendering index) for this item
  // 0 indicates center index here.
  int barindex_;

  // actual dataindex for this item
  int dataindex_;

  // select item data ptr
  void* data_;

  // check is current item is selected
  bool is_selected_;
};

/* @brief used for calculating bar position of select item
   (with very basic math expression) */
class Wheel : public BaseObject
{
public:
  Wheel();
  ~Wheel();

  void SetCenterIndex(int center_idx);

  /* @brief Clear all list items and item data. */
  void Clear();

  size_t get_select_list_size() const;
  double get_select_bar_scroll() const;

  int get_selected_index() const;

  int get_selected_dataindex() const;

  WheelItem* get_item(int index);

  /* Add wheel item data 
   * Should be suitable form to current Wheel class.
   * Allocated memory is released by Wheel class.
   * You may use NewData() method, which is more safe to use. */
  void AddData(void* data);
  virtual void Prepare(int visible_bar_count);
  virtual void RebuildItems();

  void set_infinite_scroll(bool inf_scroll);
  void ScrollDown();
  void ScrollUp();

  /* TODO: curve & rot with z pos? */

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

protected:
  // select item data
  std::vector<void*> select_data_;

  // select bar sprites
  std::vector<WheelItem*> bar_;

  // currently focused item index
  int focused_index_;

  // center index of the scroll
  int center_index_;

  // current scroll pos, relative to current focused index.
  double scroll_pos_;

  // enable for infinite scroll.
  bool inf_scroll_;

  // type of calculating select bar position
  // If false, some extern method should update bar position manually.
  int pos_method_;

  // struct for item pos parameter with expr.
  struct {
    double curve_level;
    double curve_size;
    int bar_height;
    int bar_margin;
    int bar_offset_x;
    int bar_center_y;

    struct {
      int x, y;
    } text_margin;
  } pos_expr_param_;

  // struct for item pos parameter with fixed position.
  struct {
    BaseObject tween_bar[kDefaultBarCount + 1];
    BaseObject tween_bar_focus[kDefaultBarCount + 1];
  } pos_fixed_param_;

  GameSound wheel_sound_;

  void UpdateItemPos();

  void UpdateItemPosByExpr();

  void UpdateItemPosByFixed();

  virtual void doUpdate(float delta);

  virtual void doRender();

  virtual WheelItem* CreateWheelItem(int index);
};

}