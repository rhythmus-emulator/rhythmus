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
constexpr int kScrollPosMaxDiff = 3;

enum MenuPosMethod
{
  kMenuPosExpression,
  kMenuPosFixed,
};

class MenuData
{
public:
  virtual ~MenuData() {};

  std::string name;
  std::string desc;
};

/* @brief Context of rendering select item */
class MenuItem : public Sprite
{
public:
  MenuItem();

  /**
   * @brief
   * Invalidate current item by using own property.
   * Call this method when something rendering
   * context is changed.
   * e.g. Change of song difficulty
   */
  virtual void Load();
  void set_data(MenuData* d);
  void set_focus(bool focused);
  MenuData* get_data();

private:
  // actual barindex (rendering index) for this item
  // 0 indicates center index here.
  int barindex_;

  // select item data ptr
  MenuData* data_;

  // check is current item is selected
  bool is_focused_;
};

/* @brief used for calculating bar position of select item
   (with very basic math expression) */
class Menu : public BaseObject
{
public:
  Menu();
  ~Menu();

  void AddData(MenuData* d);
  template <typename T>
  T* NewData() { T* p = new T(); AddData(p); return p; }

  MenuData& GetSelectedMenuData();
  MenuData& GetMenuDataByIndex(int index);
  MenuData* GetMenuDataByName(const std::string& name);
  void SelectMenuByName(const std::string& name);
  void SelectMenuByIndex(int index);

  /* @brief Get total data count */
  int size() const;

  /* @brief get current selected index */
  int index() const;

  /* @brief Clear all list items and item data. */
  void Clear();

  void set_display_count(int display_count);
  void set_focus_min_index(int min_index);
  void set_focus_max_index(int max_index);
  void set_focus_index(int index);
  void set_infinite_scroll(bool inf_scroll);

  /* @brief Build items to display which is suitable for current data_index.
   * This method must be called when data_index is changed. */
  virtual void RebuildItems();

  void NavigateDown();
  void NavigateUp();
  void NavigateLeft();
  void NavigateRight();

  /* TODO: curve & rot with z pos? */

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);
  virtual void Load();

protected:
  // select item data
  std::vector<MenuData*> data_;

  // select bar sprites
  std::vector<MenuItem*> bar_;

  // currently selected data index
  int data_index_;

  // displayed item count
  int display_count_;

  // focus index of the displayed item
  int focus_index_;

  // focus area of the displayed item
  int focus_min_, focus_max_;

  // current scroll delta
  double scroll_delta_;

  // enable for infinite scroll.
  bool inf_scroll_;

  // (internal use) used for menu item shifting.
  // This will reduce item invalidation, result in better performance.
  int index_delta_;

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

  // struct for item pos parameter with fixed position.
  struct {
    BaseObject tween_bar[kDefaultBarCount + 1];
    BaseObject tween_bar_focus[kDefaultBarCount + 1];
  } pos_fixed_param_;

  Sprite focus_effect_;

  Sound wheel_sound_;

  void UpdateItemPos();
  void UpdateItemPosByExpr();
  void UpdateItemPosByFixed();
  virtual void doUpdate(float delta);
  virtual void doRender();
  virtual MenuItem* CreateMenuItem();
};

}