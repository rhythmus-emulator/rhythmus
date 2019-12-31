#include "MusicWheel.h"
#include "SceneManager.h"
#include "Script.h"
#include "Player.h"
#include "Setting.h"
#include "rparser.h"

namespace rhythmus
{

MusicWheelData::MusicWheelData() : type(Songitemtype::kSongitemSong)
{
  info.difficulty = 0;
  info.level = 0;
}

void MusicWheelData::NextChart()
{
  if (charts_.empty())
    return;
  chartidx = (chartidx + 1) % charts_.size();
  ApplyFromSongListData(charts_[chartidx]);
}

void MusicWheelData::ApplyFromSongListData(SongListData &song)
{
  info = song;
  type = Songitemtype::kSongitemSong;
}

MusicWheelItem::MusicWheelItem()
{
}

/* @warn use same metric that MusicWheel used. */
void MusicWheelItem::Load(const Metric &metric)
{
  title_ = std::make_unique<Text>();
  title_->set_name("MusicWheelTitle");
  title_->LoadByName();
  title_->LoadCommandWithNamePrefix(metric);

  /* title object won't be affected by op code. */
  title_->SetIgnoreVisibleGroup(true);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    background_[i].set_name("MusicWheelType" + std::to_string(i));
    background_[i].LoadByName();
    background_[i].LoadCommandWithPrefix("Bar" + std::to_string(i), metric);
    // Call LoadFromLR2DST and clear position data after it if necessary
    // (for LR2 compatibility)
    if (true /* TODO: BarTypeSpritePosRemove metric */)
      background_[i].SetAllTweenPos(0, 0);
  }

  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
  {
    level_[i].set_name("MusicWheelLevel" + std::to_string(i));
    level_[i].LoadByName();
    level_[i].LoadCommandWithNamePrefix(metric);
  }

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
    AddChild(&background_[i]);
  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
    AddChild(&level_[i]);
  AddChild(&*title_);
  AddChild(&*level_);
}

static const int _type_to_baridx[] = {
  0,
  1,
  2,
  3,
  4,
  5,
  6,
  7,
  7,
  9
};

static const bool _type_to_disp_level[] = {
  true,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false
};

bool MusicWheelItem::LoadFromMenuData(MenuData *d)
{
  if (!MenuItem::LoadFromMenuData(d))
    return false;


  MusicWheelData *data = static_cast<MusicWheelData*>(d);

  title_->SetText(data->info.title);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    if (i == _type_to_baridx[data->type])
      background_[i].Show();
    else
      background_[i].Hide();
  }

  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
  {
    if (i == data->info.difficulty && _type_to_disp_level[data->type])
    {
      level_[i].Show();
      level_[i].SetNumber(data->info.level);
    }
    else
    {
      level_[i].Hide();
      // XXX: kind of trick
      // LR0 event causes level in not hidden state,
      // so make it empty string instead of hidden.
      level_[i].SetText(std::string());
    }
  }

  return true;
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{
  set_name("MusicWheel");
  set_infinite_scroll(true);
  sort_.type = 0;
  sort_.invalidate = true;
  filter_.gamemode = 0;
  filter_.difficulty = 0;
  filter_.invalidate = true;

  for (size_t i = 0; i < Sorttype::kSortEnd; ++i)
    sort_.avail_type[i] = 1;
  for (size_t i = 0; i < Difficulty::kDifficultyEnd; ++i)
    filter_.avail_difficulty[i] = 1;
  for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
    filter_.avail_gamemode[i] = 1;

#if 0
  set_display_count(24);
  set_focus_max_index(12);
  set_focus_min_index(12);
  set_focus_index(12);
#endif
}

MusicWheel::~MusicWheel()
{
  Clear();
}

MusicWheelData& MusicWheel::get_data(int dataindex)
{
  return static_cast<MusicWheelData&>(Menu::GetMenuDataByIndex(dataindex));
}

MusicWheelData& MusicWheel::get_selected_data(int player_num)
{
  return static_cast<MusicWheelData&>(Menu::GetSelectedMenuData());
}

MenuItem* MusicWheel::CreateMenuItem()
{
  MusicWheelItem* item = new MusicWheelItem();
  item->set_parent(this);
  return item;
}

void MusicWheel::Load(const Metric &metric)
{
  Menu::Load(metric);
  /* load system preference */
  /* TODO: limit gamemode and sort filter by metric */
  filter_.gamemode = Setting::GetSystemSetting().GetOption("Gamemode")->value<int>();
  filter_.difficulty = Setting::GetSystemSetting().GetOption("Difficultymode")->value<int>();
  filter_.invalidate = true;
  sort_.type = Setting::GetSystemSetting().GetOption("Sorttype")->value<int>();
  sort_.invalidate = true;
}

void MusicWheel::OnSelectChange(const MenuData *data, int direction)
{
  const auto *d = static_cast<const MusicWheelData*>(data);
  int flag_code[6];
  memset(flag_code, 0, sizeof(flag_code));

  /* Type of item? */
  switch (d->type)
  {
  case 0: /* General Song Item */
    flag_code[2] = 1;
    flag_code[5] = 1;
    break;
  case 1: /* Folder */
    flag_code[1] = 1;
    break;
  case 2: /* User-Customized Folder */
    flag_code[1] = 1;
    break;
  case 3: /* New song folder */
    flag_code[1] = 1;
    break;
  case 4: /* Rival folder */
    flag_code[1] = 1;
    break;
  case 5: /* Song (versus mode) */
    flag_code[2] = 1;
    flag_code[5] = 1;
    break;
  case 6: /* Course folder */
    flag_code[3] = 1;
    flag_code[5] = 1;
    break;
  case 8: /* Course */
    flag_code[3] = 1;
    flag_code[5] = 1;
    break;
  case 9: /* Random */
    flag_code[5] = 1;
    break;
  }
  for (size_t i = 1; i < 6; ++i)
    Script::getInstance().SetFlag(i, flag_code[i]);

  /* Flag for difficulty of current song. */
  Script::getInstance().SetString(10, d->info.title);
  Script::getInstance().SetString(11, d->info.subtitle);
  Script::getInstance().SetString(12, d->info.title + " " + d->info.subtitle);
  Script::getInstance().SetString(13, d->info.genre);
  Script::getInstance().SetString(14, d->info.artist);
  /* BPM */
  Script::getInstance().SetNumber(90, d->info.bpm_max);
  Script::getInstance().SetNumber(91, d->info.bpm_min);
  Script::getInstance().SetFlag(176, d->info.bpm_max == d->info.bpm_min);
  Script::getInstance().SetFlag(177, d->info.bpm_max != d->info.bpm_min);
  /* IR */
  Script::getInstance().SetNumber(92, 0);
  Script::getInstance().SetNumber(93, 0);
  Script::getInstance().SetNumber(94, 0);
  /* Song difficulty existence */
  int diff_exist[5] = { 0, 0, 0, 0, 0 };
  int diff_not_exist[5] = { 1, 1, 1, 1, 1 };
  int levels[5] = { UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX };
  int diff_type = 0;
  /* TODO: search all difficulty in songlistdata. */
  if (flag_code[2] == 1 /* if song */)
  {
    switch (d->info.difficulty)
    {
    case Difficulty::kDifficultyEasy:
      diff_exist[0] = 1;
      diff_not_exist[0] = 0;
      levels[0] = d->info.level;
      diff_type = 1;
      break;
    case Difficulty::kDifficultyNormal:
      diff_exist[1] = 1;
      diff_not_exist[1] = 0;
      levels[1] = d->info.level;
      diff_type = 2;
      break;
    case Difficulty::kDifficultyHard:
      diff_exist[2] = 1;
      diff_not_exist[2] = 0;
      levels[2] = d->info.level;
      diff_type = 3;
      break;
    case Difficulty::kDifficultyEx:
      diff_exist[3] = 1;
      diff_not_exist[3] = 0;
      levels[3] = d->info.level;
      diff_type = 4;
      break;
    case Difficulty::kDifficultyInsane:
      diff_exist[4] = 1;
      diff_not_exist[4] = 0;
      levels[4] = d->info.level;
      diff_type = 5;
      break;
    }
  }
  for (size_t i = 0; i < 5; ++i)
  {
    Script::getInstance().SetFlag(500 + i, diff_not_exist[i]);
    Script::getInstance().SetFlag(505 + i, diff_exist[i]);
    Script::getInstance().SetFlag(510 + i, diff_type > 0);  // TODO: single
    Script::getInstance().SetFlag(515 + i, 0);              // TODO: multiple
    Script::getInstance().SetNumber(45 + i, levels[i]);
    Script::getInstance().SetFlag(151 + i, diff_type - 1 == i);
  }
  Script::getInstance().SetFlag(150, diff_type == 0);

  /* Load matching playrecord */
  auto *playrecord = Player::getMainPlayer().GetPlayRecord(d->name);
  if (!playrecord)
  {
    static PlayRecord playrecord_empty;
    playrecord_empty.pg = 0;
    playrecord_empty.gr = 0;
    playrecord_empty.gd = 0;
    playrecord_empty.bd = 0;
    playrecord_empty.pr = 0;
    playrecord_empty.total_note = d->info.notecount;
    playrecord_empty.maxcombo = 0;
    playrecord_empty.option = 0;
    playrecord_empty.option_dp = 0;
    playrecord_empty.score = 0;
    playrecord_empty.playcount = 0;
    playrecord_empty.clearcount = 0;
    playrecord_empty.failcount = 0;
    playrecord = &playrecord_empty;
  }
  Script::getInstance().SetNumber(70, playrecord->score);
  Script::getInstance().SetNumber(71, playrecord->exscore());
  Script::getInstance().SetNumber(72, playrecord->total_note * 2);
  Script::getInstance().SetNumber(73, (int)playrecord->rate());
  Script::getInstance().SetNumber(74, playrecord->total_note);
  Script::getInstance().SetNumber(75, playrecord->maxcombo);
  Script::getInstance().SetNumber(76, playrecord->bd + playrecord->pr);
  Script::getInstance().SetNumber(77, playrecord->playcount);
  Script::getInstance().SetNumber(78, playrecord->clearcount);
  Script::getInstance().SetNumber(79, playrecord->failcount);
  Script::getInstance().SetNumber(80, playrecord->pg);
  Script::getInstance().SetNumber(81, playrecord->gr);
  Script::getInstance().SetNumber(82, playrecord->gd);
  Script::getInstance().SetNumber(83, playrecord->bd);
  Script::getInstance().SetNumber(84, playrecord->pr);
  Script::getInstance().SetNumber(85, static_cast<int>(playrecord->pg * 100.0f / playrecord->total_note));
  Script::getInstance().SetNumber(86, static_cast<int>(playrecord->gr * 100.0f / playrecord->total_note));
  Script::getInstance().SetNumber(87, static_cast<int>(playrecord->gd * 100.0f / playrecord->total_note));
  Script::getInstance().SetNumber(88, static_cast<int>(playrecord->bd * 100.0f / playrecord->total_note));
  Script::getInstance().SetNumber(89, static_cast<int>(playrecord->pr * 100.0f / playrecord->total_note));
  for (int i = 0; i <= 45; ++i)
    Script::getInstance().SetFlag(520 + i, 0);
  if (flag_code[2] == 1 /* if song */ && playrecord)
  {
    int clridx = playrecord->clear_type;  // XXX: need to use enum
    switch (d->info.difficulty)
    {
    case Difficulty::kDifficultyEasy:
      Script::getInstance().SetFlag(520 + clridx, 1);
      break;
    case Difficulty::kDifficultyNormal:
      Script::getInstance().SetFlag(530 + clridx, 1);
      break;
    case Difficulty::kDifficultyHard:
      Script::getInstance().SetFlag(540 + clridx, 1);
      break;
    case Difficulty::kDifficultyEx:
      Script::getInstance().SetFlag(550 + clridx, 1);
      break;
    case Difficulty::kDifficultyInsane:
      Script::getInstance().SetFlag(560 + clridx, 1);
      break;
    }
  }

  Script::getInstance().SetSliderNumber(1, data_index_ * 100.0 / size());

  EventManager::SendEvent("SongSelectChange");
  if (direction == -1)
    EventManager::SendEvent("SongSelectChangeUp");
  else if (direction == 1)
    EventManager::SendEvent("SongSelectChangeDown");
}

void MusicWheel::OnSelectChanged()
{
  EventManager::SendEvent("SongSelectChanged");
}

void MusicWheel::NavigateLeft()
{
  // Close section only if section is opened.
  if (!current_section_.empty())
  {
    CloseSection();
    RebuildData();
    OnSelectChange(&get_selected_data(0), 0);
  }
}

void MusicWheel::NavigateRight()
{
  // if selected item is folder and not opened, then go into it.
  // if selected is folder and opened, then close it.
  // if song, change select difficulty and rebuild item.
  auto &sel_data = get_selected_data(0);
  if (sel_data.type == Songitemtype::kSongitemFolder)
  {
    if (sel_data.name == current_section_)
      CloseSection();
    else
      OpenSection(sel_data.name);
  }
  else if (sel_data.type == Songitemtype::kSongitemSong)
  {
    sel_data.NextChart();
  }
  RebuildData();
  OnSelectChange(&get_selected_data(0), 0);
}

void MusicWheel::RebuildData()
{
  // filter songs
  if (filter_.invalidate)
  {
    filter_.invalidate = false;
    data_filtered_.clear();
    size_t i = 0;
    bool pass_filter;
    for (auto &song : SongList::getInstance())
    {
      pass_filter = true;
      switch (filter_.gamemode)
      {
      case Gamemode::kGamemodeNone:
        break;
      case Gamemode::kGamemode4Key:
      case Gamemode::kGamemode5Key:
      case Gamemode::kGamemode7Key:
      case Gamemode::kGamemodeDDR:
      case Gamemode::kGamemodeEZ2DJ:
      case Gamemode::kGamemodeIIDXSP:
      case Gamemode::kGamemodeIIDXDP:
        pass_filter = (song.type == filter_.gamemode);
      }
      if (!pass_filter)
        continue;

      MusicWheelData data;
      data.ApplyFromSongListData(song);
      // TODO: add songdata to existing data if same song path.
      data.type = Songitemtype::kSongitemSong;
      data_filtered_.push_back(data);
    }
  }

  // sort data object
  if (sort_.invalidate)
  {
    sort_.invalidate = false;
    switch (sort_.type)
    {
    case Sorttype::kNoSort:
      break;
    case Sorttype::kSortByLevel:
      std::sort(data_filtered_.begin(), data_filtered_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.info.level < b.info.level;
      });
      break;
    case Sorttype::kSortByTitle:
      std::sort(data_filtered_.begin(), data_filtered_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return strcmp(a.info.title.c_str(), b.info.title.c_str());
      });
      break;
    default:
      ASSERT(0);
    }
  }

  // clear items and store previously selected item.
  // previously selected item will be used to focus item
  // after section open/close.
  std::string previous_selection;
  if (!data_.empty())
    previous_selection = get_selected_data(0).name;
  else if (!current_section_.empty())
    previous_selection = current_section_;
  Clear();

  // add songs & default sections/items
  MusicWheelData sections[10];
  sections[0].name = "all_songs";
  sections[0].info.title = "All Songs";
  sections[0].type = Songitemtype::kSongitemFolder;
  sections[1].name = "new_songs";
  sections[1].info.title = "New Songs";
  sections[1].type = Songitemtype::kSongitemFolder;
  for (size_t i = 0; i < 2; ++i)
  {
    AddData(new MusicWheelData(sections[i]));
    if (sections[i].name == current_section_)
    {
      // TODO: filtering once again by section type/name
      for (auto &d : data_filtered_)
        data_.push_back(new MusicWheelData(d));
    }
  }
  
  // re-select previous selection
  data_index_ = 0;
  for (size_t i = 0; i < data_.size(); ++i)
  {
    if (data_[i]->name == previous_selection)
    {
      data_index_ = i;
      break;
    }
  }

  // rebuild rendering items
  RebuildItems();
}

void MusicWheel::OpenSection(const std::string &section)
{
  current_section_ = section;
}

void MusicWheel::CloseSection()
{
  current_section_.clear();
}

void MusicWheel::Sort(int sort)
{
  sort_.type = sort;
  Setting::GetSystemSetting().GetOption("Sorttype")->set_value(sort_.type);
  sort_.invalidate = true;
}

void MusicWheel::SetGamemodeFilter(int filter)
{
  filter_.gamemode = filter;
  Setting::GetSystemSetting().GetOption("Gamemode")->set_value(filter_.gamemode);
  filter_.invalidate = true;
}

void MusicWheel::SetDifficultyFilter(int filter)
{
  filter_.difficulty = filter;
  Setting::GetSystemSetting().GetOption("Difficultymode")->set_value(filter_.difficulty);
  filter_.invalidate = true;
}

void MusicWheel::NextSort()
{
  for (size_t i = 0; i < Sorttype::kSortEnd; ++i)
  {
    sort_.type = (sort_.type + 1) % Sorttype::kSortEnd;
    if (sort_.avail_type[sort_.type])
      break;
  }
  Sort(sort_.type);
}

void MusicWheel::NextGamemodeFilter()
{
  for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
  {
    filter_.gamemode = (filter_.gamemode + 1) % Gamemode::kGamemodeEnd;
    if (filter_.avail_gamemode[filter_.gamemode])
      break;
  }
  SetGamemodeFilter(filter_.gamemode);
}

void MusicWheel::NextDifficultyFilter()
{
  for (size_t i = 0; i < Difficulty::kDifficultyEnd; ++i)
  {
    filter_.difficulty = (filter_.difficulty + 1) % Difficulty::kDifficultyEnd;
    if (filter_.avail_difficulty[filter_.difficulty])
      break;
  }
  SetDifficultyFilter(filter_.difficulty);
}

int MusicWheel::GetSort() const
{
  return sort_.type;
}

int MusicWheel::GetDifficultyFilter() const
{
  return filter_.difficulty;
}

}