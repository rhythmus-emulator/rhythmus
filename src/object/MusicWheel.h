#pragma once

#include "Song.h"
#include "Menu.h"
#include "Text.h"
#include "Number.h"
#include <string>

namespace rhythmus
{

enum Songitemtype
{
  kSongitemSong,
  kSongitemFolder,
  kSongitemCustomFolder,
  kSongitemNewFolder,
  kSongitemRivalFolder,
  kSongitemRivalSong,
  kSongitemCourseFolder,
  kSongitemCourse,
  kSongitemRandomSong,
  kSongitemRandomCourse,
  kSongitemEnd,
};

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
  virtual void OnSelectChange(const MenuData *data, int direction);
  virtual void OnSelectChanged();
  virtual void RebuildData();

  void OpenSection(const std::string &section);
  void Sort(int sort);
  void SetGamemodeFilter(int filter);
  void SetDifficultyFilter(int filter);
  void NextSort();
  void NextGamemodeFilter();
  void NextDifficultyFilter();
  int GetSort() const;
  int GetDifficultyFilter() const;

  friend class MusicWheelItem;

private:
  struct {
    int type;
    bool invalidate;
  } sort_;
  struct {
    int gamemode;
    int difficulty;
    bool invalidate;
  } filter_;
  std::string current_section_;
  std::vector<MusicWheelData> data_filtered_;

  virtual MenuItem* CreateMenuItem();
};

}