#pragma once

#include "Scene.h"

namespace rhythmus
{

struct SelectItem
{
  std::string name;
  std::string artist;
};

class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void StartScene();
  virtual void CloseScene();
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

private:
  // total select item list
  std::vector<SelectItem> select_list_;

  // currently selected item index
  int select_index_;

  // current scroll pos
  double select_scroll_pos_;

  // scrolling speed
  double select_scroll_speed_;

  void UpdateScrollPos();
};

}