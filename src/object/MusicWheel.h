#pragma once

#include "Song.h"
#include "Game.h"
#include "ListView.h"
#include "Text.h"
#include "Number.h"
#include "KeyPool.h"
#include <memory>
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

struct MusicWheelData;

struct MusicWheelSongData
{
  std::vector<MusicWheelData*> charts;
};

struct MusicWheelData
{
  SongListData info;
  std::string name;
  std::string sectionname;
  int type;
  const PlayRecord *record;
  int clear;
  double rate;
  size_t chartidx;

  MusicWheelData();
  void ApplyFromSongListData(SongListData &song);
  void SetSection(const std::string &sectionname, const std::string &title);
  void SetPlayRecord();
};

/* @brief Pure music wheel item interface */
class MusicWheelItem : public ListViewItem
{
public:
  MusicWheelItem();
  virtual void Load(const MetricGroup &metric);
  virtual void LoadFromData(void *d);
  Sprite *get_background(unsigned type);

private:
  Sprite background_[NUM_SELECT_BAR_TYPES];
  Number level_[NUM_LEVEL_TYPES];
  std::unique_ptr<Text> title_;
};

class MusicWheel : public ListView
{
public:
  MusicWheel();
  ~MusicWheel();

  virtual void Load(const MetricGroup &metric);

  MusicWheelData *get_data(int dataindex);
  MusicWheelData *get_selected_data(int player_num);
  MusicWheelItem *get_wheel_item(unsigned itemindex);
  unsigned get_wheel_item_count() const;

  virtual void OnSelectChange(const void *data, int direction);
  virtual void OnSelectChanged();
  virtual void NavigateLeft();
  virtual void NavigateRight();
  virtual void RebuildData();
  virtual void RebuildDataContent(ListViewData &data);

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
  /* data for song list. */
  std::vector<MusicWheelSongData> data_songs_;

  MetricGroup item_metric;

  /* Keypools updated by MusicWheel object */
  KeyData<std::string> info_title;
  KeyData<std::string> info_subtitle;
  KeyData<std::string> info_fulltitle;
  KeyData<std::string> info_genre;
  KeyData<std::string> info_artist;
  KeyData<int> info_itemtype;
  KeyData<int> info_diff;
  KeyData<int> info_bpmmax;
  KeyData<int> info_bpmmin;
  KeyData<int> info_level;
  KeyData<int> info_difftype_1;
  KeyData<int> info_difftype_2;
  KeyData<int> info_difftype_3;
  KeyData<int> info_difftype_4;
  KeyData<int> info_difftype_5;
  KeyData<int> info_difflv_1;
  KeyData<int> info_difflv_2;
  KeyData<int> info_difflv_3;
  KeyData<int> info_difflv_4;
  KeyData<int> info_difflv_5;
  KeyData<int> info_score;
  KeyData<int> info_exscore;
  KeyData<int> info_totalnote;
  KeyData<int> info_maxcombo;
  KeyData<int> info_playcount;
  KeyData<int> info_clearcount;
  KeyData<int> info_failcount;
  KeyData<int> info_cleartype;
  KeyData<int> info_pg;
  KeyData<int> info_gr;
  KeyData<int> info_gd;
  KeyData<int> info_bd;
  KeyData<int> info_pr;
  KeyData<float> info_musicwheelpos;
};

}