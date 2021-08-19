#pragma once

#include "Wheel.h"
#include "KeyPool.h"
#include <memory>
#include <string>

namespace rhythmus
{
  
class PlayRecord;
struct LR2WheelData;
class LR2WheelItem;

class LR2MusicWheel : public Wheel
{
public:
  LR2MusicWheel();
  ~LR2MusicWheel();

  virtual void OnReady();
  using Wheel::RunCommandLR2;
  virtual void RunCommandLR2(const std::string& command, const LR2FnArgs& args);
  virtual void OnSelectChange(const void* data, int direction);
  virtual void OnSelectChanged();
  virtual void MoveDown();
  virtual void MoveUp();
  virtual void Expand();
  virtual void Collapse();
  virtual const std::string& GetSelectedItemId() const;
  virtual void SetSelectedItemId(const std::string& id);
  // TODO: folder expanding status

  void SetSort(int sort);
  void SetGamemodeFilter(int filter);
  void SetDifficultyFilter(int filter);
  void NextSort();
  void NextGamemodeFilter();
  void NextDifficultyFilter();

  void RebuildItem();
  void ClearData();

private:
  int sort_;

  struct {
    int gamemode;
    int key;
    int difficulty;
  } filter_;

  bool invalidate_data_;

  std::string current_section_;

  unsigned bar_center_;

  unsigned data_index_;

  double scroll_time_;
  double scroll_;

  /* section items (created by default) */
  std::vector<LR2WheelData*> data_sections_;

  /* displayed data */
  std::vector<LR2WheelData*> data_;

  /* displayed items */
  std::vector<LR2WheelItem*> items_;

  /* dummy items for positioning */
  std::vector<BaseObject*> pos_on_;
  std::vector<BaseObject*> pos_off_;

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

  virtual void doUpdate(double delta);

  friend class LR2MusicWheelLR2Handler;
};

}