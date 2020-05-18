#include "MusicWheel.h"
#include "SceneManager.h"
#include "Script.h"
#include "Player.h"
#include "Setting.h"
#include "rparser.h"
#include "common.h"

namespace rhythmus
{

// ----------------------------- MusicWheelData

MusicWheelData::MusicWheelData()
  : type(Songitemtype::kSongitemSong), record(nullptr), clear(0), rate(.0)
{
  info.difficulty = 0;
  info.level = 0;
}

void MusicWheelData::NextChart()
{
  if (charts_.empty())
    return;
  chartidx = (chartidx + 1) % charts_.size();
  ApplyFromSongListData(*charts_[chartidx]);
}

void MusicWheelData::ApplyFromSongListData(SongListData &song)
{
  name = song.id;
  info = song;
  type = Songitemtype::kSongitemSong;
  SetPlayRecord();
}

void MusicWheelData::SetSection(const std::string &sectionname, const std::string &title)
{
  name = sectionname;
  info.title = title;
  type = Songitemtype::kSongitemFolder;
}

void MusicWheelData::SetPlayRecord()
{
  auto &player = *PlayerManager::GetPlayer();
  record = nullptr;
  if (type == Songitemtype::kSongitemSong
   || type == Songitemtype::kSongitemCourse)
    record = player.GetPlayRecord(info.id);
  if (!record)
  {
    clear = ClearTypes::kClearNone;
    rate = .0;
  }
  else
  {
    clear = record->clear_type;
    rate = record->rate();
  }
}

void MusicWheelData::AddChart(SongListData *d)
{
  charts_.push_back(d);
}

// ----------------------------- MusicWheelItem

MusicWheelItem::MusicWheelItem()
{
}

/* @warn use same metric that MusicWheel used. */
void MusicWheelItem::Load(const MetricGroup &metric)
{
  title_ = std::make_unique<Text>();
  title_->set_name("MusicWheelTitle");
  title_->Load(metric);

  /* title object won't be affected by op code. */
  //title_->SetIgnoreVisibleGroup(true);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    background_[i].set_name("MusicWheelType" + std::to_string(i));
    background_[i].Load(metric);
  }

  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
  {
    level_[i].set_name("MusicWheelLevel" + std::to_string(i));
    level_[i].Load(metric);
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

MusicWheel::MusicWheel() :
  info_title(KEYPOOL->GetString("MusicWheelTitle")),
  info_subtitle(KEYPOOL->GetString("MusicWheelSubTitle")),
  info_fulltitle(KEYPOOL->GetString("MusicWheelFullTitle")),
  info_genre(KEYPOOL->GetString("MusicWheelGenre")),
  info_artist(KEYPOOL->GetString("MusicWheelArtist")),
  info_itemtype(KEYPOOL->GetInt("MusicWheelType")),
  info_diff(KEYPOOL->GetInt("MusicWheelDiff")),
  info_bpmmax(KEYPOOL->GetInt("MusicWheelBpmMax")),
  info_bpmmin(KEYPOOL->GetInt("MusicWheelBpmMin")),
  info_level(KEYPOOL->GetInt("MusicWheelLevel")),
  info_difftype_1(KEYPOOL->GetInt("MusicWheelDT1")),
  info_difftype_2(KEYPOOL->GetInt("MusicWheelDT2")),
  info_difftype_3(KEYPOOL->GetInt("MusicWheelDT3")),
  info_difftype_4(KEYPOOL->GetInt("MusicWheelDT4")),
  info_difftype_5(KEYPOOL->GetInt("MusicWheelDT5")),
  info_difflv_1(KEYPOOL->GetInt("MusicWheelDLV1")),
  info_difflv_2(KEYPOOL->GetInt("MusicWheelDLV2")),
  info_difflv_3(KEYPOOL->GetInt("MusicWheelDLV3")),
  info_difflv_4(KEYPOOL->GetInt("MusicWheelDLV4")),
  info_difflv_5(KEYPOOL->GetInt("MusicWheelDLV5")),
  info_score(KEYPOOL->GetInt("MusicWheelScore")),
  info_exscore(KEYPOOL->GetInt("MusicWheelExScore")),
  info_totalnote(KEYPOOL->GetInt("MusicWheelTotalNote")),
  info_maxcombo(KEYPOOL->GetInt("MusicWheelMaxCombo")),
  info_playcount(KEYPOOL->GetInt("MusicWheelPlayCount")),
  info_clearcount(KEYPOOL->GetInt("MusicWheelClearCount")),
  info_failcount(KEYPOOL->GetInt("MusicWheelFailCount")),
  info_cleartype(KEYPOOL->GetInt("MusicWheelClearType")),
  info_pg(KEYPOOL->GetInt("MusicWheelPG")),
  info_gr(KEYPOOL->GetInt("MusicWheelGR")),
  info_gd(KEYPOOL->GetInt("MusicWheelGD")),
  info_bd(KEYPOOL->GetInt("MusicWheelBD")),
  info_pr(KEYPOOL->GetInt("MusicWheelPR")),
  info_musicwheelpos(KEYPOOL->GetFloat("MusicWheelPos"))
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

void MusicWheel::Load(const MetricGroup &metric)
{
  /* create section datas */
  MusicWheelData section;
  section.SetSection("all_songs", "All Songs");
  data_sections_.push_back(section);
  section.SetSection("new_songs", "New Songs");
  data_sections_.push_back(section);

  /* load system preference */
  filter_.gamemode = *PREFERENCE->gamemode;
  filter_.difficulty = *PREFERENCE->select_difficulty_mode;
  filter_.invalidate = true;
  sort_.type = *PREFERENCE->select_sort_type;
  sort_.invalidate = true;

  /* limit gamemode and sort filter types by metric */
  if (metric.exist("GamemodeFilter"))
  {
    for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
      filter_.avail_gamemode[i] = 0;
    CommandArgs filters(metric.get<std::string>("GamemodeFilter"));
    for (size_t i = 0; i < filters.size(); ++i)
    {
      filter_.avail_gamemode[
        StringToGamemode(filters.Get<std::string>(i).c_str())] = 1;
    }
    if (filter_.avail_gamemode[filter_.gamemode] == 0)
      filter_.gamemode = Gamemode::kGamemodeNone;
  }
  if (metric.exist("DifficultyFilter"))
  {
    for (size_t i = 0; i < Difficulty::kDifficultyEnd; ++i)
      filter_.avail_difficulty[i] = 0;
    CommandArgs diffs(metric.get<std::string>("DifficultyFilter"));
    for (size_t i = 0; i < diffs.size(); ++i)
    {
      filter_.avail_difficulty[
        StringToDifficulty(diffs.Get<std::string>(i).c_str())] = 1;
    }
    if (filter_.avail_difficulty[filter_.difficulty] == 0)
      filter_.difficulty = Difficulty::kDifficultyNone;
  }
  if (metric.exist("SortType"))
  {
    for (size_t i = 0; i < Sorttype::kSortEnd; ++i)
      sort_.avail_type[i] = 0;
    CommandArgs sorts(metric.get<std::string>("SortType"));
    for (size_t i = 0; i < sorts.size(); ++i)
    {
      sort_.avail_type[
        StringToSorttype(sorts.Get<std::string>(i).c_str())] = 1;
    }
    if (sort_.avail_type[sort_.type] == 0)
      sort_.type = Sorttype::kNoSort;
  }

  Menu::Load(metric);
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

void MusicWheel::OnSelectChange(const MenuData *data, int direction)
{
  const auto *d = static_cast<const MusicWheelData*>(data);


  // update KeyPool
  *info_title = d->info.title;
  *info_subtitle = d->info.subtitle;
  *info_fulltitle = d->info.title + " " + d->info.subtitle;
  *info_genre = d->info.genre;
  *info_artist = d->info.artist;
  *info_diff = d->info.difficulty;
  *info_itemtype = d->type;
  *info_bpmmax = d->info.bpm_max;
  *info_bpmmin = d->info.bpm_min;
  *info_level = d->info.level;


  /* TODO: Song difficulty existence
  int diff_exist[5] = { 0, 0, 0, 0, 0 };
  int diff_type[5] = { 2, 2, -1, 2 ,3 };
  uint32_t levels[5] = { 3, 6, 9, 11, 12 };
   */
  *info_difftype_1 = 1;
  *info_difftype_2 = 2;
  *info_difftype_3 = 2;
  *info_difftype_4 = 4;
  *info_difftype_5 = 5;
  *info_difflv_1 = 2;
  *info_difflv_2 = 3;
  *info_difflv_3 = 8;
  *info_difflv_4 = 11;
  *info_difflv_5 = 12;


  // Load matching playrecord
  auto *playrecord = PlayerManager::GetPlayer()->GetPlayRecord(d->name);
  if (playrecord)
  {
    *info_score = 100000;
    *info_exscore = 1234;
    *info_totalnote = 3000;
    *info_maxcombo = 1000;
    *info_playcount = 2;
    *info_clearcount = 1;
    *info_failcount = 1;
    *info_cleartype = 2;
    *info_pg = 1000;
    *info_gr = 234;
    *info_gd = 0;
    *info_bd = 0;
    *info_pr = 0;
  }


  // update pos
  *info_musicwheelpos = data_index_ * 100.0 / (float)size();


  // send event
  EventManager::SendEvent("SongSelectChanged");
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
    RebuildData();
  }
  OnSelectChange(&get_selected_data(0), 0);
}

void MusicWheel::RebuildData()
{
  // filter songs
  if (filter_.invalidate)
  {
    filter_.invalidate = false;
    charts_.clear();
    data_filtered_.clear();
    size_t i = 0;
    bool pass_filter;

    // gamemode filtering
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
      case Gamemode::kGamemodeIIDX5Key:
      case Gamemode::kGamemodeIIDX10Key:
        pass_filter = (song.type == filter_.gamemode);
      }
      if (!pass_filter)
        continue;
      charts_.push_back(&song);
    }

    // difficulty filtering
    for (auto *c : charts_)
    {
      auto &sdata = *static_cast<SongListData*>(c);
      if (filter_.difficulty != Difficulty::kDifficultyNone
       && sdata.difficulty != filter_.difficulty)
        continue;

      MusicWheelData data;
      data.ApplyFromSongListData(sdata);
      data.type = Songitemtype::kSongitemSong;
      data_filtered_.push_back(data);
    }

    // add difficulty info into chartlist.
    // TODO: need to set chartidx
    for (auto &d : data_filtered_)
    {
      for (auto *c : charts_)
      {
        auto &sdata = *static_cast<SongListData*>(c);
        if (sdata.songpath == d.info.songpath)
          d.AddChart(&sdata);
      }
    }

    // XXX: need to send in sort invalidation
    EventManager::SendEvent("SongFilterChanged");
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
        return a.info.title < b.info.title;
      });
      break;
    case Sorttype::kSortByClear:
      std::sort(data_filtered_.begin(), data_filtered_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.clear < b.clear;
      });
      break;
    case Sorttype::kSortByRate:
      std::sort(data_filtered_.begin(), data_filtered_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.rate < b.rate;
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
  for (size_t i = 0; i < data_sections_.size(); ++i)
  {
    AddData(new MusicWheelData(data_sections_[i]));
    if (data_sections_[i].name == current_section_)
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
  RebuildData();
}

void MusicWheel::CloseSection()
{
  // restore selected index from section index
  for (size_t i = 0; i < data_sections_.size(); ++i)
  {
    if (data_sections_[i].name == current_section_)
    {
      data_index_ = i;
      break;
    }
  }
  current_section_.clear();
  RebuildData();
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

int MusicWheel::GetGamemode() const
{
  return filter_.gamemode;
}

int MusicWheel::GetDifficultyFilter() const
{
  return filter_.difficulty;
}

}
