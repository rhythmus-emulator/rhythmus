#pragma once

#include "Scene.h"

namespace rhythmus
{

constexpr int NUM_SELECT_BAR_TYPES = 10;
constexpr int NUM_SELECT_BAR_DISPLAY_MAX = 50;

/* @brief Actual data which select item uses when rendering */
struct SelectItemData
{
  int type;
  std::string name;
  std::string artist;
};

/* @brief Context of rendering select item */
struct SelectItem
{
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
class SelectBarPosition
{
public:
  SelectBarPosition();
  void SetCenterIndex(int center_idx);
  void SetCurveLevel(double curve_level);
  void SetBarPosition(int offset, int align = 2 /* 0 left, 1 center, 2 right */);
  void SetBarHeight(int height);
  void SetBarMargin(int margin);
  void SetTextMargin(double x, double y);
  virtual void GetTweenInfo(double pos_idx, TweenInfo &out);
  virtual void GetTextTweenInfo(double pos_idx, TweenInfo &out);

  /* TODO: curve & rot with z pos? */

protected:
  int center_idx_;
  double curve_level_;
  double curve_size_;
  int bar_height_;
  int bar_margin_;
  int bar_offset_x_;
  double text_margin_x_, text_margin_y_;
};

class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void StartScene();
  virtual void CloseScene();
  virtual void Update();
  virtual void Render();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual const std::string GetSceneName() const;

  /* Get selected title in current scene. */
  const char* get_selected_title() const;

  /* Get selected index */
  int get_selected_index() const;

  /* Get selected bar title by index.
   * This function mainly used by LR2SelectBarSprite class */
  const char* get_select_bar_title(int select_bar_idx) const;

  /* Get selection scroll. */
  double get_select_bar_scroll() const;

  /* Get select list size */
  int get_select_list_size() const;

  void DrawSelectBar();

  void SetSelectBarPosition(SelectBarPosition* position_object = nullptr);
  SelectBarPosition* GetSelectBarPosition();
  void SetSelectBarImage(int type_no, ImageAuto img);

private:
  // src set sprite
  SpriteAuto select_bar_src_[NUM_SELECT_BAR_TYPES];
/*
  std::vector<SpriteAnimation> select_bar_pos_;
  SpriteAnimation select_text_;
  SpriteAnimation select_focus_text_;
*/

  // select item data
  std::vector<SelectItemData> select_data_;

  // select bar item for rendering
  std::vector<SelectItem> select_bar_;

  // currently focused item index
  int focused_index_;

  // center index of the scroll
  int center_index_;

  // current scroll pos, relative to current focused index.
  double scroll_pos_;

  // wheel items to draw
  int select_bar_draw_count_;

  // bar position calculator
  std::unique_ptr<SelectBarPosition> bar_position_;

  // prepare select item pools
  void InitializeItems();

  // fill render context to select item
  virtual void RebuildItems();

  // update item pos (should be called in Update())
  virtual void UpdateItemPos();
};

}