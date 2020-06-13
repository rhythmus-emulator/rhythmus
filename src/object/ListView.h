#pragma once

#include "Sprite.h"
#include "Font.h"
#include "Sound.h"
#include <string>
#include <memory>

namespace rhythmus
{

constexpr int NUM_SELECT_BAR_TYPES = 10;
constexpr int NUM_LEVEL_TYPES = 7;
constexpr int kDefaultBarCount = 30;

enum class ListViewType
{
  kList,
  kWheel,
  kIcon,
  kEnd
};

enum MenuPosMethod
{
  kMenuPosExpression,
  kMenuPosFixed,
};

/* @brief Context of rendering select item */
class ListViewItem : public BaseObject
{
public:
  ListViewItem();
  ListViewItem(const ListViewItem& obj);

  virtual void Load(const MetricGroup &m);
  virtual void LoadFromData(void *data);
  void set_dataindex(unsigned dataindex);
  unsigned get_dataindex() const;
  void set_focus(bool focused);
  void* get_data();
  DrawProperty &get_item_dprop();

private:
  unsigned dataindex_;

  // select item data ptr
  void* data_;

  // check is current item is selected
  bool is_focused_;

  // property to be used for drawn.
  DrawProperty item_dprop_;

  virtual void OnAnimation(DrawProperty &frame);
};

/* @brief used for calculating bar position of select item
   (with very basic math expression) */
class ListView : public BaseObject
{
public:
  ListView();
  ~ListView();

  virtual void Load(const MetricGroup &metric);

  void* GetSelectedMenuData();
  void* GetMenuDataByIndex(int index);
  void SelectMenuByIndex(int index);

  /* @brief Get total data count */
  unsigned size() const;

  /* @brief get current selected index */
  unsigned index() const;

  /* @brief Clear all list items and item data. */
  void Clear();
  void AddData(void* d);

  void set_listviewtype(ListViewType type);
  void set_item_min_index(unsigned min_index);
  void set_item_max_index(unsigned max_index);
  void set_item_center_index(unsigned index);

  bool is_wheel() const;

  /* @brief Rebuild source data. */
  virtual void RebuildData();

  /* @brief Build items to display which is suitable for current data_index.
   * This method must be called when data_index is changed. */
  virtual void RebuildItems();

  virtual void NavigateDown();
  virtual void NavigateUp();
  virtual void NavigateLeft();
  virtual void NavigateRight();

  /* TODO: curve & rot with z pos? */

  virtual void OnSelectChange(const void *data, int direction);
  virtual void OnSelectChanged();

protected:
  // select item data
  std::vector<void*> data_;

  // select bar items to draw
  std::vector<ListViewItem*> items_;

  // select bar items (with absolute index)
  std::vector<ListViewItem*> items_abs_;

  // current top data position of listview
  // @warn may be negative or bigger than data size.
  int data_top_index_;

  // currently selected data index
  // @warn may be negative or bigger than data size.
  int data_index_;

  // item display type
  ListViewType viewtype_;

  // displayed item count
  unsigned item_count_;

  // calculate item count automatically
  bool set_item_count_auto_;

  // center of the item index (which is being selected)
  unsigned item_center_index_;

  // focus area of the displayed item
  unsigned item_focus_min_, item_focus_max_;

  // basic item height
  unsigned item_height_;

  // ListViewItem type
  std::string item_type_;

  // scroll time
  float scroll_time_;

  // scroll time (remain)
  float scroll_time_remain_;

  // current scroll delta ( used by UpdateItemPos() )
  float scroll_delta_;

  // type of calculating select bar position
  // If false, some extern method should update bar position manually.
  int pos_method_;

  // struct for item pos parameter with expr.
  struct {
    double curve_level;
    double curve_size;
    int bar_width;
    int bar_height;
    int bar_margin;
    int bar_offset_x;
    int bar_center_y;

    struct {
      int x, y;
    } text_margin;
  } pos_expr_param_;

  Sprite focus_effect_;

  Sound wheel_sound_;

  unsigned CalculateItemCount() const;
  void UpdateItemPos();
  void UpdateItemPosByExpr();
  void UpdateItemPosByFixed();
  virtual void doUpdate(double);
  virtual ListViewItem* CreateMenuItem(const std::string &itemtype);
};

}
