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
class SelectItem : public Sprite
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

  void SetSelectBarImage(int type_no, ImageAuto img);
  void UpdateItemPos();
  virtual void Prepare(int visible_bar_count);
  virtual void RebuildItems();

  void ScrollDown();
  void ScrollUp();
#if 0
  virtual void GetTweenInfo(double pos_idx, TweenInfo &out);
  virtual void GetTextTweenInfo(double pos_idx, TweenInfo &out);
#endif

  /* TODO: curve & rot with z pos? */

protected:
  int center_idx_;
  double curve_level_;
  double curve_size_;
  int bar_height_;
  int bar_margin_;
  int bar_offset_x_;
  double text_margin_x_, text_margin_y_;

  // textures for each bar type
  SpriteAuto select_bar_src_[NUM_SELECT_BAR_TYPES];

  // select item data
  std::vector<SelectItemData> select_data_;

  // select bars (TODO: use SelectWheelItem)
  std::vector<SelectItem*> bar_;

  // currently focused item index
  int focused_index_;

  // center index of the scroll
  int center_index_;

  // current scroll pos, relative to current focused index.
  double scroll_pos_;

  virtual void SetItemPosByIndex(SelectItem& item, int idx, double r);
  virtual void doUpdate();

  // XXX: on test purpose
  friend class SelectScene;
};

class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual const std::string GetSceneName() const;

  SelectWheel& get_wheel();

private:
/*
  std::vector<SpriteAnimation> select_bar_pos_;
  SpriteAnimation select_text_;
  SpriteAnimation select_focus_text_;
*/
  SelectWheel wheel_;
};

}