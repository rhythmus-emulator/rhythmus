#include "MusicWheel.h"
#include "SceneManager.h"
#include "Script.h"
#include "Setting.h"

namespace rhythmus
{

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

  /* title object don't affected by op code. */
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

  // TODO: Set NumberText or NumberSprite.
  /* TODO: NumberSpriteLevel metric */
  level_ = std::make_unique<Number>();
  level_->set_name("MusicWheelLevel0");
  level_->LoadByName();
  level_->LoadCommandWithNamePrefix(metric);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    AddChild(&background_[i]);
  }
  AddChild(&*title_);
  AddChild(&*level_);
}

bool MusicWheelItem::LoadFromMenuData(MenuData *d)
{
  if (!MenuItem::LoadFromMenuData(d))
    return false;

  MusicWheelData *data = static_cast<MusicWheelData*>(d);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    if (i == data->type)
      background_[i].Show();
    else
      background_[i].Hide();
  }

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    // TODO: display proper bar type
  }

  title_->SetText(data->title);
  level_->SetNumber(data->level);

  return true;
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{
  set_name("MusicWheel");
  set_infinite_scroll(true);
  sort_.type = 0;
  sort_.invalidate = false;
  filter_.gamemode = 0;
  filter_.difficulty = 0;
  filter_.invalidate = false;
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

  /* Song stat */
  Script::getInstance().SetString(10, d->title);
  Script::getInstance().SetString(11, d->subtitle);
  Script::getInstance().SetString(12, d->title + " " + d->subtitle);
  Script::getInstance().SetString(13, d->genre);
  Script::getInstance().SetString(14, d->artist);
  Script::getInstance().SetNumber(160, 140);
  Script::getInstance().SetNumber(160, 140);
  Script::getInstance().SetNumber(70, 180000);
  Script::getInstance().SetNumber(71, 2880);
  Script::getInstance().SetNumber(72, 2880);
  Script::getInstance().SetNumber(73, 100);
  Script::getInstance().SetNumber(74, 1235);
  Script::getInstance().SetNumber(75, 1234);
  Script::getInstance().SetNumber(76, 9);
  Script::getInstance().SetNumber(77, 5);
  Script::getInstance().SetNumber(78, 3);
  Script::getInstance().SetNumber(79, 1);
  /* Playrecord */
  Script::getInstance().SetNumber(80, 1000);
  Script::getInstance().SetNumber(81, 300);
  Script::getInstance().SetNumber(82, 100);
  Script::getInstance().SetNumber(83, 12);
  Script::getInstance().SetNumber(84, 9);
  Script::getInstance().SetNumber(85, 90);
  Script::getInstance().SetNumber(86, 10);
  Script::getInstance().SetNumber(87, 5);
  Script::getInstance().SetNumber(88, 3);
  Script::getInstance().SetNumber(89, 2);
  /* BPM */
  Script::getInstance().SetNumber(90, 140);
  Script::getInstance().SetNumber(91, 140);
  /* IR */
  Script::getInstance().SetNumber(92, 0);
  Script::getInstance().SetNumber(93, 0);
  Script::getInstance().SetNumber(94, 0);

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
      data.name = song.title;
      data.title = song.title;
      data.artist = song.artist;
      data.subtitle = song.subtitle;
      data.subartist = song.subartist;
      data.songpath = song.songpath;
      data.chartname = song.chartpath;
      data.type = Songitemtype::kSongitemSong;
      data.level = song.level;
      data.index = i++;
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
        return a.level < b.level;
      });
      break;
    case Sorttype::kSortByName:
      std::sort(data_filtered_.begin(), data_filtered_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return strcmp(a.title.c_str(), b.title.c_str());
      });
      break;
    default:
      ASSERT(0);
    }
  }

  // clear items and store previously selected item
  std::string previous_selection;
  previous_selection = get_selected_data(0).name;
  Clear();

  // add songs & default sections/items
  MusicWheelData sections[10];
  sections[0].name = "all_songs";
  sections[0].title = "All Songs";
  sections[0].type = Songitemtype::kSongitemFolder;
  sections[0].name = "new_songs";
  sections[1].title = "New Songs";
  sections[1].type = Songitemtype::kSongitemFolder;
  for (size_t i = 0; i < 2; ++i)
  {
    if (sections[i].name == current_section_)
    {
      // TODO: filtering once again by section type/name
      for (auto &d : data_filtered_)
        data_.push_back(new MusicWheelData(d));
    }
  }
  
  // re-select previous selection
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
  UpdateItemPos();
}

void MusicWheel::OpenSection(const std::string &section)
{
  current_section_ = section;
}

void MusicWheel::Sort(int sort)
{
  sort_.type = sort;
  sort_.invalidate = true;
}

void MusicWheel::SetGamemodeFilter(int filter)
{
  filter_.gamemode = filter;
  filter_.invalidate = true;
}

void MusicWheel::SetDifficultyFilter(int filter)
{
  filter_.difficulty = filter;
  filter_.invalidate = true;
}

void MusicWheel::NextSort()
{
  sort_.type = (sort_.type + 1) % Sorttype::kSortEnd;
  sort_.invalidate = true;
}

void MusicWheel::NextGamemodeFilter()
{
  filter_.gamemode = (filter_.gamemode + 1) % Gamemode::kGamemodeEnd;
  filter_.invalidate = true;
}

void MusicWheel::NextDifficultyFilter()
{
  filter_.difficulty = (filter_.difficulty + 1) % Difficulty::kDifficultyEnd;
  filter_.invalidate = true;
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