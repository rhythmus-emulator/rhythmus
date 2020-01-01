#pragma once

#include "Song.h"
#include "Menu.h"
#include "Text.h"
#include "Number.h"
#include <string>

namespace rhythmus
{

class PlayRecord;

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
  MusicWheelData();
  SongListData info;
  std::string sectionname;
  int type;
  const PlayRecord *record;
  int clear;
  double rate;

  void NextChart();
  void ApplyFromSongListData(SongListData &song);
  void SetSection(const std::string &sectionname, const std::string &title);
  void SetPlayRecord();
  void AddChart(SongListData *d);

private:
  size_t chartidx;
  std::vector<SongListData*> charts_;
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
  Number level_[NUM_LEVEL_TYPES];
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
  virtual void NavigateLeft();
  virtual void NavigateRight();
  virtual void RebuildData();

  void OpenSection(const std::string &section);
  void CloseSection();
  void Sort(int sort);
  void SetGamemodeFilter(int filter);
  void SetDifficultyFilter(int filter);
  void NextSort();
  void NextGamemodeFilter();
  void NextDifficultyFilter();
  int GetSort() const;
  int GetGamemode() const;
  int GetDifficultyFilter() const;

  friend class MusicWheelItem;

private:
  struct {
    int type;
    int avail_type[Sorttype::kSortEnd];
    bool invalidate;
  } sort_;
  struct {
    int gamemode;
    int difficulty;
    int avail_gamemode[Gamemode::kGamemodeEnd];
    int avail_difficulty[Difficulty::kDifficultyEnd];
    bool invalidate;
  } filter_;
  std::string current_section_;

  /* source charts (only filtered by gamemode) */
  std::vector<void*> charts_;
  /* filtered items */
  std::vector<MusicWheelData> data_filtered_;
  /* section items (created in Load procedure) */
  std::vector<MusicWheelData> data_sections_;

  virtual MenuItem* CreateMenuItem();
};

}