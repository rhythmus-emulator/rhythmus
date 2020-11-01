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
    std::string attrname = format_string("lr2bar%dsrc", i);
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
  //AddChild(&*title_); -- TODO: this method makes double-free with unique destructor. need to be fixed
  //AddChild(&*level_);
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

void MusicWheelItem::LoadFromData(void *d)
{
  MusicWheelData *data = static_cast<MusicWheelData*>(d);
  
  // TODO: if data is nullptr, then clear all data and set as empty item
  if (data == nullptr)
  {
    if (title_) title_->Hide();
    for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
      background_[i].Hide();
    for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
      level_[i].Hide();
    return;
  }

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
}

Sprite *MusicWheelItem::get_background(unsigned type)
{
  return &background_[type];
}

void MusicWheelItem::doRender()
{
  WheelItem::doRender();
  GRAPHIC->DrawRect(0, 0, 200, 60, 0x66FF9999);
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
  ClearData();
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

  /* copy item metric for creation */
  if (metric.get_group("item"))
    item_metric = *metric.get_group("item");

  /* limit gamemode and sort filter types by metric */
  if (metric.exist("GamemodeFilter"))
  {
    for (size_t i = 0; i < Gamemode::kGamemodeEnd; ++i)
      filter_.avail_gamemode[i] = 0;
    CommandArgs filters(metric.get_str("GamemodeFilter"));
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
    CommandArgs diffs(metric.get_str("DifficultyFilter"));
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
    CommandArgs sorts(metric.get_str("SortType"));
    for (size_t i = 0; i < sorts.size(); ++i)
    {
      sort_.avail_type[
        StringToSorttype(sorts.Get<std::string>(i).c_str())] = 1;
    }
    if (sort_.avail_type[sort_.type] == 0)
      sort_.type = Sorttype::kNoSort;
  }

  Wheel::Load(metric);
}

MusicWheelData* MusicWheel::get_data(int dataindex)
{
  return static_cast<MusicWheelData*>(Wheel::GetMenuDataByIndex(dataindex));
}

MusicWheelData* MusicWheel::get_selected_data(int player_num)
{
  return static_cast<MusicWheelData*>(Wheel::GetSelectedMenuData());
}

void MusicWheel::OnSelectChange(const void *data, int direction)
{
  const auto *d = static_cast<const MusicWheelData*>(data);

  if (d == nullptr) {
    static MusicWheelData data_empty;
    d = &data_empty;
  }

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
  *info_musicwheelpos = static_cast<float>(data_index_ * 100.0 / size());


  // send event
  EVENTMAN->SendEvent("SongSelectChanged");
  if (direction == -1)
    EVENTMAN->SendEvent("SongSelectChangeUp");
  else if (direction == 1)
    EVENTMAN->SendEvent("SongSelectChangeDown");
}

void MusicWheel::OnSelectChanged()
{
  EVENTMAN->SendEvent("SongSelectChanged");
}

void MusicWheel::NavigateLeft()
{
  // Close section only if section is opened.
  if (!current_section_.empty())
  {
    CloseSection();
    OnSelectChange(get_selected_data(0), 0);
  }
}

void MusicWheel::NavigateRight()
{
  // if selected item is folder and not opened, then go into it.
  // if selected is folder and opened, then close it.
  // if song, change select difficulty and rebuild item.
  auto *sel_data = get_selected_data(0);
  if (!sel_data) return;

  if (sel_data->type == Songitemtype::kSongitemFolder)
  {
    if (sel_data->name == current_section_)
      CloseSection();
    else
      OpenSection(sel_data->name);
  }
  else if (sel_data->type == Songitemtype::kSongitemSong)
  {
    NextDifficultyFilter();
  }
  OnSelectChange(get_selected_data(0), 0);
}

void MusicWheel::RebuildData()
{
  // clear items and store previously selected item here.
  // previously selected item will be used to focus item
  // after section open/close.
  std::string previous_selection;
  if (!data_.empty())
    previous_selection = get_selected_data(0)->name;
  else if (!current_section_.empty())
    previous_selection = current_section_;
  ClearData();
  data_filtered_.clear();
  data_songs_.clear();

  // filter songs
  if (filter_.invalidate)
  {
    size_t i = 0;
    bool pass_filter;
    std::map<std::string, MusicWheelSongData *> song_chart_map;

    filter_.invalidate = false;
    charts_.clear();
    data_filtered_.clear();

    // gamemode filtering
    for (auto &song : *SONGLIST)
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

    // create song list
    for (auto &c : data_filtered_)
    {
      if (song_chart_map.find(c.info.songpath) == song_chart_map.end())
      {
        MusicWheelSongData d;
        data_songs_.push_back(d);
        song_chart_map[c.info.songpath] = &data_songs_.back();
      }
      song_chart_map[c.info.songpath]->charts.push_back(&c);
    }

    // XXX: need to send in sort invalidation
    EVENTMAN->SendEvent("SongFilterChanged");
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
      R_ASSERT(0);
    }
  }

  // add songs & default sections/items
  for (size_t i = 0; i < data_sections_.size(); ++i)
  {
    AddData(&data_sections_[i]);
    if (data_sections_[i].name == current_section_)
    {
      // TODO: filtering once again by section type/name
      for (auto &d : data_filtered_)
        AddData(&d);
    }
  }
  
  // re-select previous selection
  data_index_ = 0;
  for (size_t i = 0; i < data_.size(); ++i)
  {
    if (static_cast<MusicWheelData*>(data_[i].p)->name == previous_selection)
    {
      data_index_ = i;
      break;
    }
  }

  // rebuild rendering items
  RebuildItems();
}

void MusicWheel::RebuildDataContent(WheelItemData &data)
{
  // create from MusicWheel.item data
  // so metric must be alive after object initialization
  if (!data.content)
  {
    MusicWheelItem *item = new MusicWheelItem();
    item->set_name("MusicWheelItem");
    item->LoadFromName();
    item->set_parent(this);
    data.content = item;
  }
}

WheelItem *MusicWheel::CreateWheelWrapper()
{
  // TODO: check item_type_ == "LR2"
  return new MusicWheelItem();
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
  // change difficulty filter
  for (size_t i = 0; i < Difficulty::kDifficultyEnd; ++i)
  {
    filter_.difficulty = (filter_.difficulty + 1) % Difficulty::kDifficultyEnd;
    if (filter_.avail_difficulty[filter_.difficulty])
      break;
  }
  SetDifficultyFilter(filter_.difficulty);
  // TODO: set song index
  // if next song difficulty is merged into single item,
  // then replace current data with next difficulty item.
  // else, search another difficulty item and set data index with that.
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

// -------------------------------------------------------------------- Loaders

class LR2CSVMusicWheelHandlers
{
public:
  static void src_bar_body(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    unsigned bgtype = 0;

    /* for first-appearence */
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return;
    auto *wheel = (MusicWheel*)scene->FindChildByName("MusicWheel");
    if (!wheel) return;
    if (loader->get_object("musicwheel") == NULL)
    {
      wheel->BringToTop();
      wheel->SetWheelWrapperCount(30, "LR2");
      wheel->SetWheelPosMethod(WheelPosMethod::kMenuPosFixed);
      loader->set_object("musicwheel", wheel);
    }

    bgtype = (unsigned)ctx->get_int(1);
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i)
    {
      auto *item = static_cast<MusicWheelItem*>(wheel->GetMenuItemWrapperByIndex(i));
      auto *bg = item->get_background(bgtype);
      LR2CSVExecutor::CallHandler("#SRC_IMAGE", bg, loader, ctx);
    }
  }

  static void dst_bar_body_off(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = (MusicWheel*)loader->get_object("musicwheel");
    unsigned itemindex = 0;
    std::string itemname;
    if (!wheel) return;
    itemindex = (unsigned)ctx->get_int(1);
    itemname = format_string("musicwheelitem%u", itemindex);
    // XXX: Sprite casting is okay?
    auto *item = (Sprite*)wheel->GetMenuItemWrapperByIndex(itemindex);
    LR2CSVExecutor::CallHandler("#DST_IMAGE", item, loader, ctx);
  }

  static void dst_bar_body_on(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO: implement. currently ignored.
  }

  static void bar_center(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    /* for first-appearence */
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return;
    auto *wheel = (MusicWheel*)scene->FindChildByName("MusicWheel");
    if (!wheel) return;
    loader->set_object("musicwheel", wheel);

    wheel->set_item_center_index((unsigned)ctx->get_int(1));
  }

  static void bar_available(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    /* for first-appearence */
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return;
    auto *wheel = (MusicWheel*)scene->FindChildByName("MusicWheel");
    if (!wheel) return;
    loader->set_object("musicwheel", wheel);

    wheel->set_item_min_index(0);
    wheel->set_item_max_index((unsigned)ctx->get_int(1));
  }

  static void src_bar_title(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_bar_title(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_bar_flash(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_bar_flash(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_bar_level(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_bar_level(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_my_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_my_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_rival_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_rival_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void src_rival_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  static void dst_rival_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {

  }

  LR2CSVMusicWheelHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BAR_BODY", (LR2CSVCommandHandler)&src_bar_body);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_OFF", (LR2CSVCommandHandler)&dst_bar_body_off);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_ON", (LR2CSVCommandHandler)&dst_bar_body_on);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVCommandHandler)&bar_center);
    LR2CSVExecutor::AddHandler("#BAR_AVAILABLE", (LR2CSVCommandHandler)&bar_available);
    LR2CSVExecutor::AddHandler("#SRC_BAR_TITLE", (LR2CSVCommandHandler)&src_bar_title);
    LR2CSVExecutor::AddHandler("#DST_BAR_TITLE", (LR2CSVCommandHandler)&dst_bar_title);
    LR2CSVExecutor::AddHandler("#SRC_BAR_FLASH", (LR2CSVCommandHandler)&src_bar_flash);
    LR2CSVExecutor::AddHandler("#DST_BAR_FLASH", (LR2CSVCommandHandler)&dst_bar_flash);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LEVEL", (LR2CSVCommandHandler)&src_bar_level);
    LR2CSVExecutor::AddHandler("#DST_BAR_LEVEL", (LR2CSVCommandHandler)&dst_bar_level);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LAMP", (LR2CSVCommandHandler)&src_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_BAR_LAMP", (LR2CSVCommandHandler)&dst_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_MY_BAR_LAMP", (LR2CSVCommandHandler)&src_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_MY_BAR_LAMP", (LR2CSVCommandHandler)&dst_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_BAR_LAMP", (LR2CSVCommandHandler)&src_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_BAR_LAMP", (LR2CSVCommandHandler)&dst_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_LAMP", (LR2CSVCommandHandler)&src_rival_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_LAMP", (LR2CSVCommandHandler)&dst_rival_lamp);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVCommandHandler)&bar_center);
  }
};

// register handlers
LR2CSVMusicWheelHandlers _LR2CSVMusicWheelHandlers;

}