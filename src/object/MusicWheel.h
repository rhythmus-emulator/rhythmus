#pragma once

#include "Song.h"
#include "Menu.h"
#include "Text.h"
#include <string>

namespace rhythmus
{

class MusicWheelData : public MenuData
{
public:
  std::string title;
  std::string subtitle;
  std::string genre;
  std::string artist;
  std::string subartist;
  std::string songpath;
  std::string chartname;
  int type;
  int level;
  int index;
};

/* @brief Pure music wheel item interface (no inheritance) */
class MusicWheelItem : public MenuItem
{
public:
  MusicWheelItem();
  Text& title();
  NumberText& level();
  MusicWheelData* get_data();
  virtual void Load();

protected:
  Text title_;
  NumberText level_;
};

class MusicWheel : public Menu
{
public:
  MusicWheel();
  MusicWheelData& get_data(int dataindex);
  MusicWheelData& get_selected_data(int player_num);

  void LoadProperty(const std::string& prop_name, const std::string& value);

private:
  // textures for each bar type
  Sprite* select_bar_src_[NUM_SELECT_BAR_TYPES];

  std::string title_font_;
  BaseObject title_dst_;

  virtual MenuItem* CreateMenuItem();
};

}