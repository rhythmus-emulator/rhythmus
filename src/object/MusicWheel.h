#pragma once

#include "Song.h"
#include "Wheel.h"
#include <string>

namespace rhythmus
{

struct MusicWheelItemData
{
  std::string title;
  std::string subtitle;
  std::string genre;
  std::string artist;
  std::string subartist;
  int type;
  int level;
  void *song;
};

/* @brief Pure music wheel item interface (no inheritance) */
class MusicWheelItem : public WheelItem
{
public:
  MusicWheelItem(int index);
  Text& title();
  Text& level();
  MusicWheelItemData* get_data();
  virtual void Invalidate();

protected:
  Text title_;
  Text level_;
};

class MusicWheel : public Wheel
{
public:
  MusicWheel();
  MusicWheelItem& get_item(int index);
  MusicWheelItemData& get_data(int dataindex);
  MusicWheelItemData& get_selected_data();

  /* @brief Add new item data and get it's pointer. */
  MusicWheelItemData& NewData();

  void LoadProperty(const std::string& prop_name, const std::string& value);

private:
  // textures for each bar type
  Sprite* select_bar_src_[NUM_SELECT_BAR_TYPES];

  virtual WheelItem* CreateWheelItem(int index);
};

}