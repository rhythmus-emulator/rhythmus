#pragma once

#include "Song.h"
#include "Game.h"
#include "Wheel.h"
#include "Text.h"
#include "Number.h"
#include "KeyPool.h"
#include <memory>
#include <string>

namespace rhythmus
{

class PlayRecord;

enum class MusicWheelDataType
{
  None,
  Song,
  Folder,
  CustomFolder,
  NewFolder,
  RivalFolder,
  RivalSong,
  CourseFolder,
  Course,
  RandomSong,
  RandomCourse
};

class MusicWheelData
{
public:
  MusicWheelData();
  MusicWheelData(MusicWheelDataType type, const std::string& id, const std::string& name, bool is_section);
  MusicWheelData(const ChartMetaData* c);

  void SetFromChart(const ChartMetaData* chart);
  void SetAsSection(const std::string& name);
  void SetRandom(bool is_random);
  const ChartMetaData* GetChart() const;
  void NextChart();
  void SetNextChartId(const std::string& id);
  std::string GetNextChartId() const;
  void set_id(const std::string& id);
  const std::string& get_id() const;
  const std::string& get_parent_id() const;
  void set_type(MusicWheelDataType type);
  MusicWheelDataType get_type() const;
  bool IsSection() const;
  bool IsRandom() const;

  std::string title;        // title for displaying
  int level;
  int diff;
  int clear;
  double rate;
  std::string songpath;     // for filtering and grouping

private:
  const ChartMetaData* chart_;
  MusicWheelDataType type_;
  std::string id_;          // unique ID
  std::string parent_id_;   // parent ID for section
  std::string next_id_;     // for moving to next item
  bool is_section_;
  bool is_random_;
};

/* @brief music wheel item wrapper */
class MusicWheelItem : public WheelItem
{
public:
  MusicWheelItem();
  virtual void Load(const MetricGroup &metric);
  virtual void LoadFromData(void *d);

  Sprite *get_background(unsigned type);
  Number *get_level(unsigned type);
  Text *get_title();

private:
  Sprite background_[NUM_SELECT_BAR_TYPES];
  Number level_[NUM_LEVEL_TYPES];
  Text title_;

  virtual void doUpdate(double delta);
  virtual void doRender();
};

class MusicWheel : public Wheel
{
public:
  MusicWheel();
  ~MusicWheel();

  virtual void Load(const MetricGroup &metric);
  void InitializeLR2();

  MusicWheelData *get_data(int dataindex);
  MusicWheelData *get_selected_data(int player_num);

  virtual void OnSelectChange(const void *data, int direction);
  virtual void OnSelectChanged();
  virtual void NavigateLeft();
  virtual void NavigateRight();
  virtual void RebuildData();
  virtual void RebuildDataContent(WheelItemData &data);
  virtual WheelItem *CreateWheelWrapper();

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
    int key;
    int difficulty;
    bool invalidate;
  } filter_;
  std::string current_section_;

  /* display item per chart. if not, per song. */
  bool item_per_chart_;

  /* filtered items */
  std::vector<MusicWheelData> data_charts_;
  /* section items (created by default) */
  std::vector<MusicWheelData> data_sections_;

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