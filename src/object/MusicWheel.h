#pragma once

#include "Song.h"
#include "Menu.h"
#include "Text.h"
#include "Number.h"
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

/* @brief Pure music wheel item interface */
class MusicWheelItem : public MenuItem
{
public:
  MusicWheelItem();
  virtual void Load(const Metric &metric);
  virtual bool LoadFromMenuData(MenuData *d);

private:
  Sprite background_[NUM_SELECT_BAR_TYPES];
  std::unique_ptr<BaseObject> level_;
  std::unique_ptr<Text> title_;
};

class MusicWheel : public Menu
{
public:
  MusicWheel();
  ~MusicWheel();

  MusicWheelData &get_data(int dataindex);
  MusicWheelData &get_selected_data(int player_num);

  virtual void Load(const Metric &metric);
  friend class MusicWheelItem;

private:
  virtual MenuItem* CreateMenuItem();
};

}