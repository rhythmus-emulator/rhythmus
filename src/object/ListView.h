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
  kMenuPosFixed,    // for fixed position by index for each listview items
  kMenuPosDynamic,  // for dynamic menu height object
  kMenuPosExpr,     // expression based menu position
};

struct ListViewPos
{
  int index_i;    // index of integer
  float index_f;  // index of decimal
  float y;        // current y pos
};

struct ListViewItemPos
{
  unsigned index;
  float y;
  DrawProperty p;
};

struct ListViewData
{
  unsigned index;
  float y, height;
  void *p;
  BaseObject *content;
  Point margin;
};

/* @brief Context of rendering select item */
class ListViewItem : public BaseObject
{
public:
  ListViewItem();
  ListViewItem(const ListViewItem& obj);
  virtual BaseObject *clone();

  virtual void Load(const MetricGroup &m);
  virtual void LoadFromData(void *data);
  void LoadFromLVData(const ListViewData& lvdata);

  void* get_data();
  DrawProperty &get_item_dprop();

private:
  void set_dataindex(unsigned dataindex);
  unsigned get_dataindex() const;
  void set_itemindex(int itemindex);
  int get_itemindex() const;
  void set_content(BaseObject *content);
  void set_focus(bool focused);

  friend class ListView;

private:
  unsigned dataindex_;  // index of actual data
  int itemindex_;       // index number of listview item (no duplication)

  // select item data ptr
  void* data_;

  // check is current item is selected
  bool is_focused_;

  // listview item content
  BaseObject *content_;

  // property to be used for drawn.
  DrawProperty item_dprop_;

  // margin of a item
  Point margin_;

  virtual void OnAnimation(DrawProperty &frame);
};

/*
 * @brief ListView based on recycling ListViewItem.
 *
 * @comments
 * Implementing list looping with dynamic height is almost impossible.
 * Maybe panel object need to be created to implement it.
 */
class ListView : public BaseObject
{
public:
  ListView();
  ~ListView();

  virtual void Load(const MetricGroup &metric);

  /* @brief Get total data count */
  unsigned size() const;

  /* @brief get current selected index */
  unsigned index() const;

  bool is_wheel() const;


  /* @brief Add data to be displayed.
   * @warn  RebuildItems() must be called after calling this method.
   *        Also, pointer object is not owned(deleted) by ListView. */
  void AddData(void* d);
  void AddData(void* d, const Point &margin);
  void ClearData();

  void* GetSelectedMenuData();
  void* GetMenuDataByIndex(int index);
  void SelectMenuByIndex(int index);
  float GetItemYPosByIndex(int index);

  /* @brief Rebuild source data. */
  virtual void RebuildData();

  /* @brief Build items to display which is suitable for current data_index.
   * This method must be called when data_index is changed. */
  virtual void RebuildItems();

  ListViewItem *GetLVItem(unsigned data_index);
  virtual BaseObject *CreateLVItemContent(void *data);

  void set_listviewtype(ListViewType type);
  void set_item_min_index(unsigned min_index);
  void set_item_max_index(unsigned max_index);
  void set_item_center_index(unsigned index);

  virtual void NavigateDown();
  virtual void NavigateUp();
  virtual void NavigateLeft();
  virtual void NavigateRight();

  /* TODO: curve & rot with z pos? */

  virtual void OnSelectChange(const void *data, int direction);
  virtual void OnSelectChanged();

protected:
  // current listview top item position
  ListViewPos pos_;
  MenuPosMethod pos_method_;

  // previous listview item position for checking item rebuild
  int index_current_;
  int index_previous_;

  // listview item data
  std::vector<ListViewData> data_;

  // listview item wrapper
  std::vector<ListViewItem*> items_;

  // position data. used for ypos based object.
  std::vector<ListViewItemPos> item_pos_list_;

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

  // total item height
  unsigned item_total_height_;

  // ListViewItem type
  std::string item_type_;

  // scroll from ~ to
  int scroll_idx_from_, scroll_idx_to_;

  // scroll time
  float scroll_time_;

  // scroll time (remain)
  float scroll_time_remain_;

  // struct for item pos parameter with expr.
  struct {
    double curve_level;
    double curve_size;
  } pos_expr_param_;

  Sprite focus_effect_;

  Sound wheel_sound_;

  unsigned CalculateItemCount() const;
  void UpdateItemPos();
  virtual void doUpdate(double);
};

}
