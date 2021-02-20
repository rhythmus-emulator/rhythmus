#include "MusicWheel.h"
#include "SceneManager.h"
#include "Script.h"
#include "Player.h"
#include "Setting.h"
#include "rparser.h"
#include "common.h"

namespace rhythmus
{

unsigned _wheel_index = 0;
std::string _GenerateNewId()
{
  static char buffer[10];
  sprintf(buffer, "%04u", _wheel_index);
  _wheel_index++;
  return buffer;
}

// ----------------------------- MusicWheelData

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

MusicWheelData::MusicWheelData() :
  level(0), diff(0), clear(0), rate(.0),
  chart_(nullptr), type_(MusicWheelDataType::None), is_section_(false), is_random_(false)
{
  id_ = _GenerateNewId();
}

MusicWheelData::MusicWheelData(
  MusicWheelDataType type, const std::string& id,
  const std::string& name, bool is_section) :
  title(name), level(0), diff(0), clear(0), rate(.0),
  chart_(nullptr), type_(type), id_(id), is_section_(is_section), is_random_(false)
{
}

MusicWheelData::MusicWheelData(const ChartMetaData* c) :
  level(0), diff(0), clear(0), rate(.0),
  chart_(c), type_(MusicWheelDataType::Song), is_section_(false), is_random_(false)
{
  SetFromChart(c);
}

void MusicWheelData::SetFromChart(const ChartMetaData* chart)
{
  is_section_ = false;
  if (chart == nullptr) {
    title = "(None)";
    level = 0;
    diff = 0;
    clear = 0;
    chart_ = nullptr;
    type_ = MusicWheelDataType::None;
    return;
  }
  chart_ = chart;
  id_ = chart->id;
  title = chart->title;
  level = chart->level;
  diff = chart->difficulty;
  clear = ClearTypes::kClearNone;
  rate = .0;
  songpath = chart->songpath;

  auto* playrecord = PlayerManager::GetPlayer()->GetPlayRecord(id_);
  if (playrecord) {
    clear = playrecord->clear_type;
    rate = playrecord->rate();
  }
}

void MusicWheelData::SetAsSection(const std::string& name)
{
  is_section_ = true;
  title = name;
  level = 0;
  diff = 0;
  clear = 0;
  rate = .0;
  chart_ = nullptr;
  type_ = MusicWheelDataType::Folder;
}

void MusicWheelData::SetRandom(bool is_random)
{
  is_random_ = is_random;
}

const ChartMetaData* MusicWheelData::GetChart() const
{
  return chart_;
}

void MusicWheelData::NextChart()
{
  if (!chart_ || !chart_->next) return;
  SetFromChart(chart_->next);
}

void MusicWheelData::SetNextChartId(const std::string& id)
{
  next_id_ = id;
}

std::string MusicWheelData::GetNextChartId() const
{
  return next_id_;
}

void MusicWheelData::set_id(const std::string& id)
{
  id_ = id;
}

const std::string& MusicWheelData::get_id() const
{
  return id_;
}

const std::string& MusicWheelData::get_parent_id() const
{
  return parent_id_;
}

void MusicWheelData::set_type(MusicWheelDataType type)
{
  type_ = type;
}

MusicWheelDataType MusicWheelData::get_type() const
{
  return type_;
}

bool MusicWheelData::IsSection() const { return is_section_; }
bool MusicWheelData::IsRandom() const { return is_random_; }

// ----------------------------- MusicWheelItem

MusicWheelItem::MusicWheelItem()
{
  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
    AddChild(&background_[i]);
  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
    AddChild(&level_[i]);
  AddChild(&title_);
}

/* @warn use same metric that MusicWheel used. */
void MusicWheelItem::Load(const MetricGroup &metric)
{
  title_.set_name("MusicWheelTitle");
  title_.Load(metric);

  /* title object is not affected by op code. */
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
}

static const int _type_to_baridx[] = {
  0,  /* None */
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
  unsigned item_type = (unsigned)MusicWheelDataType::None;
  
  // TODO: if data is nullptr, then clear all data and set as empty item
  if (data == nullptr)
  {
    title_.Hide();
    for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
      background_[i].Hide();
    for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
      level_[i].Hide();
    return;
  }

  item_type = (unsigned)data->get_type();
  title_.SetText(data->title);
  title_.Show();

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    if (i == _type_to_baridx[item_type])
      background_[i].Show();
    else
      background_[i].Hide();
  }

  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
  {
    if (i == data->diff && _type_to_disp_level[item_type])
    {
      level_[i].Show();
      level_[i].SetNumber(data->level);
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

Sprite *MusicWheelItem::get_background(unsigned type) { return &background_[type]; }
Number *MusicWheelItem::get_level(unsigned type) { return &level_[type]; }
Text *MusicWheelItem::get_title() { return &title_; }

void MusicWheelItem::doUpdate(double delta)
{
  float w, h;
  WheelItem::doUpdate(delta);
  w = rhythmus::GetWidth(frame_.pos);
  h = rhythmus::GetHeight(frame_.pos);
  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i) {
    background_[i].SetWidth(w);
    background_[i].SetHeight(h);
  }
}

void MusicWheelItem::doRender()
{
  WheelItem::doRender();
  //GRAPHIC->DrawRect(0, 0, 200, 60, 0x66FF9999);
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
  item_per_chart_ = true;

  for (size_t i = 0; i < Sorttype::kSortEnd; ++i)
    sort_.avail_type[i] = 1;

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
  data_sections_.clear();
  data_sections_.push_back(
    MusicWheelData(MusicWheelDataType::Folder, "all_songs", "All Songs", true)
  );
  data_sections_.push_back(
    MusicWheelData(MusicWheelDataType::Folder, "new_songs", "New Songs", true)
  );

  /* load system preference */
  filter_.gamemode = PrefValue<int>("gamemode").get();
  filter_.difficulty = PrefValue<int>("difficulty").get();
  filter_.invalidate = true;
  filter_.key = 0;
  sort_.type = PrefValue<int>("sortmode").get();
  sort_.invalidate = true;

  /* copy item metric for creation */
  //if (metric.get_group("item"))
  //  item_metric = *metric.get_group("item");

  /* limit gamemode and sort filter types by metric */
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

void MusicWheel::InitializeLR2()
{
  MusicWheelData section;

  /* create section datas */
  data_sections_.clear();
  data_sections_.push_back(
    MusicWheelData(MusicWheelDataType::Folder, "all_songs", "All Songs", true)
  );
  data_sections_.push_back(
    MusicWheelData(MusicWheelDataType::Folder, "new_songs", "New Songs", true)
  );

  /* load system preference */
  filter_.gamemode = PrefValue<int>("gamemode").get();
  filter_.difficulty = PrefValue<int>("difficulty").get();
  filter_.invalidate = true;
  filter_.key = 0;
  sort_.type = PrefValue<int>("sortmode").get();
  sort_.invalidate = true;

  {
    for (size_t i = 0; i < Sorttype::kSortEnd; ++i)
      sort_.avail_type[i] = 0;
    CommandArgs sorts("none,title,level,clear");
    for (size_t i = 0; i < sorts.size(); ++i)
    {
      sort_.avail_type[
        StringToSorttype(sorts.Get<std::string>(i).c_str())] = 1;
    }
    if (sort_.avail_type[sort_.type] == 0)
      sort_.type = Sorttype::kNoSort;
  }
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
  auto* songinfo = d->GetChart();
  if (songinfo) {
    *info_title = songinfo->title;
    *info_subtitle = songinfo->subtitle;
    *info_fulltitle = songinfo->title + " " + songinfo->subtitle;
    *info_genre = songinfo->genre;
    *info_artist = songinfo->artist;
    *info_diff = songinfo->difficulty;
    *info_bpmmax = songinfo->bpm_max;
    *info_bpmmin = songinfo->bpm_min;
    *info_level = songinfo->level;
  }
  else {
    *info_title = "";
    *info_subtitle = "";
    *info_fulltitle = "";
    *info_genre = "";
    *info_artist = "";
    *info_diff = 0;
    *info_bpmmax = 0;
    *info_bpmmin = 0;
    *info_level = 0;
  }
  *info_itemtype = (int)d->get_type();


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
  auto *playrecord = PlayerManager::GetPlayer()->GetPlayRecord(d->get_id());
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

  if (sel_data->IsSection())
  {
    if (sel_data->get_id() == current_section_)
      CloseSection();
    else
      OpenSection(sel_data->get_id());
  }
  else if (sel_data->get_type() == MusicWheelDataType::Song)
  {
    NextDifficultyFilter();
  }
  OnSelectChange(get_selected_data(0), 0);
}

void MusicWheel::RebuildData()
{
  std::string previous_selection;

  // clear wheel items and store previously selected item here.
  // previously selected item will be used to focus item
  // after section open/close.
  if (!data_.empty())
    previous_selection = get_selected_data(0)->get_id();
  else if (!current_section_.empty())
    previous_selection = current_section_;
  ClearData();

  // filter songs -- invalidates chart data
  if (filter_.invalidate) {
    std::vector<const SongMetaData*> songlist;
    std::vector<const ChartMetaData*> chartlist;
    size_t i = 0;
    bool pass_filter;

    filter_.invalidate = false;
    data_charts_.clear(); // TODO: free memory; ClearMusicWheelData()

    // song filtering: gamemode
    for (const auto *song : SONGLIST->GetSongList()) {
      pass_filter = true;

      switch (filter_.gamemode) {
        case Gamemode::kGamemodeNone:
          break;
        default:
          pass_filter = (song->type == filter_.gamemode);
          break;
      }

      if (pass_filter) songlist.push_back(song);
    }

    // chart filtering: key, difficulty
    for (const auto* song : songlist) {
      ChartMetaData* c = song->chart;
      MusicWheelData* prev_chart = nullptr;
      while (c) {
        pass_filter = (filter_.key == 0 || c->key == filter_.key);
        pass_filter &= (filter_.difficulty == Difficulty::kDifficultyNone ||
          c->difficulty == filter_.difficulty);

        // XXX: is chartlist necessary?
        if (!pass_filter) chartlist.push_back(c);

        // add backptr for selecting next difficulty
        if (prev_chart) {
          prev_chart->SetNextChartId(c->id);
        }
        data_charts_.push_back(MusicWheelData(c));
        prev_chart = &data_charts_.back();

        if (!item_per_chart_) break;
        c = c->next;
        if (c == song->chart) break;
      }
    }

    // XXX: need to send in sort invalidation
    KEYPOOL->GetInt("difficulty").set(filter_.difficulty);
    KEYPOOL->GetInt("gamemode").set(filter_.gamemode);
    KEYPOOL->GetInt("sortmode").set(sort_.type);
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
      std::sort(data_charts_.begin(), data_charts_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.level < b.level;
      });
      break;
    case Sorttype::kSortByTitle:
      std::sort(data_charts_.begin(), data_charts_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.title < b.title;
      });
      break;
    case Sorttype::kSortByClear:
      std::sort(data_charts_.begin(), data_charts_.end(),
        [](const MusicWheelData &a, const MusicWheelData &b) {
        return a.clear < b.clear;
      });
      break;
    case Sorttype::kSortByRate:
      std::sort(data_charts_.begin(), data_charts_.end(),
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
    if (data_sections_[i].get_id() == current_section_)
    {
      // TODO: filtering once again by section type/name
      for (auto &d : data_charts_)
        AddData(&d);
    }
  }
  
  // re-select previous selection
  data_index_ = 0;
  for (size_t i = 0; i < data_.size(); ++i)
  {
    if (static_cast<MusicWheelData*>(data_[i].p)->get_id() == previous_selection)
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
    if (data_sections_[i].get_id() == current_section_)
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
  filter_.gamemode = (filter_.gamemode + 1) % Gamemode::kGamemodeEnd;
  SetGamemodeFilter(filter_.gamemode);
}

void MusicWheel::NextDifficultyFilter()
{
  filter_.difficulty = (filter_.difficulty + 1) % Difficulty::kDifficultyEnd;
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
  static MusicWheel *get_musicwheel(LR2CSVExecutor *loader)
  {
    auto *wheel = (MusicWheel*)loader->get_object("musicwheel");
    if (loader->get_object("musicwheel") == NULL)
    {
      /* for first-appearence */
      auto *scene = (Scene*)loader->get_object("scene");
      if (!scene) return nullptr;
      wheel = (MusicWheel*)scene->FindChildByName("MusicWheel");
      if (!wheel) return nullptr;
      wheel->InitializeLR2();
      wheel->BringToTop();
      wheel->SetWheelWrapperCount(30, "LR2");
      wheel->SetWheelPosMethod(WheelPosMethod::kMenuPosFixed);
      loader->set_object("musicwheel", wheel);
    }
    return wheel;
  }

  static bool src_bar_body(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = get_musicwheel(loader);
    unsigned bgtype = 0;
    if (!wheel) return true;
    bgtype = (unsigned)ctx->get_int(1);
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto *item = static_cast<MusicWheelItem*>(wheel->GetMenuItemWrapperByIndex(i));
      auto *bg = item->get_background(bgtype);
      LR2CSVExecutor::CallHandler("#SRC_IMAGE", bg, loader, ctx);
    }
    return true;
  }

  static bool dst_bar_body_off(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    std::string itemname;
    if (!wheel) return true;
    itemindex = (unsigned)ctx->get_int(1);
    itemname = format_string("musicwheelitem%u", itemindex);
    // XXX: Sprite casting is okay?
    auto *item = (Sprite*)wheel->GetMenuItemWrapperByIndex(itemindex);
    LR2CSVExecutor::CallHandler("#DST_IMAGE", item, loader, ctx);
    return true;
  }

  static bool dst_bar_body_on(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO: implement. currently ignored.
    return true;
  }

  static bool bar_center(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = get_musicwheel(loader);
    unsigned center_idx = 0;

    center_idx = (unsigned)ctx->get_int(1);
    wheel->set_item_center_index(center_idx);
    wheel->set_item_min_index(center_idx);
    wheel->set_item_max_index(center_idx);
    return true;
  }

  static bool bar_available(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO
    auto *wheel = get_musicwheel(loader);
    loader->set_object("musicwheel", wheel);
    //wheel->set_item_min_index(0);
    //wheel->set_item_max_index((unsigned)ctx->get_int(1));
    return true;
  }

  static bool src_bar_title(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    itemindex = (unsigned)ctx->get_int(1);
    if (itemindex != 0) return true; // TODO: bar title for index 1
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto *item = (MusicWheelItem*)wheel->GetMenuItemWrapperByIndex(i);
      auto *title = item->get_title();
      LR2CSVExecutor::CallHandler("#SRC_TEXT", title, loader, ctx);
    }
    return true;
  }

  static bool dst_bar_title(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *wheel = get_musicwheel(loader);
    unsigned itemindex = 0;
    itemindex = (unsigned)ctx->get_int(1);
    if (itemindex != 0) return true; // TODO: bar title for index 1
    for (unsigned i = 0; i < wheel->GetMenuItemWrapperCount(); ++i) {
      auto *item = (MusicWheelItem*)wheel->GetMenuItemWrapperByIndex(i);
      auto *title = item->get_title();
      loader->set_object("text", title);
      LR2CSVExecutor::CallHandler("#DST_TEXT", title, loader, ctx);
    }
    return true;
  }

  static bool src_bar_flash(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_bar_flash(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool src_bar_level(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_bar_level(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool src_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool src_my_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_my_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool src_rival_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_rival_bar_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool src_rival_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  static bool dst_rival_lamp(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    return true;
  }

  LR2CSVMusicWheelHandlers()
  {
    LR2CSVExecutor::AddHandler("#SRC_BAR_BODY", (LR2CSVHandlerFunc)&src_bar_body);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_OFF", (LR2CSVHandlerFunc)&dst_bar_body_off);
    LR2CSVExecutor::AddHandler("#DST_BAR_BODY_ON", (LR2CSVHandlerFunc)&dst_bar_body_on);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVHandlerFunc)&bar_center);
    LR2CSVExecutor::AddHandler("#BAR_AVAILABLE", (LR2CSVHandlerFunc)&bar_available);
    LR2CSVExecutor::AddHandler("#SRC_BAR_TITLE", (LR2CSVHandlerFunc)&src_bar_title);
    LR2CSVExecutor::AddHandler("#DST_BAR_TITLE", (LR2CSVHandlerFunc)&dst_bar_title);
    LR2CSVExecutor::AddHandler("#SRC_BAR_FLASH", (LR2CSVHandlerFunc)&src_bar_flash);
    LR2CSVExecutor::AddHandler("#DST_BAR_FLASH", (LR2CSVHandlerFunc)&dst_bar_flash);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LEVEL", (LR2CSVHandlerFunc)&src_bar_level);
    LR2CSVExecutor::AddHandler("#DST_BAR_LEVEL", (LR2CSVHandlerFunc)&dst_bar_level);
    LR2CSVExecutor::AddHandler("#SRC_BAR_LAMP", (LR2CSVHandlerFunc)&src_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_BAR_LAMP", (LR2CSVHandlerFunc)&dst_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_MY_BAR_LAMP", (LR2CSVHandlerFunc)&src_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_MY_BAR_LAMP", (LR2CSVHandlerFunc)&dst_my_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_BAR_LAMP", (LR2CSVHandlerFunc)&src_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_BAR_LAMP", (LR2CSVHandlerFunc)&dst_rival_bar_lamp);
    LR2CSVExecutor::AddHandler("#SRC_RIVAL_LAMP", (LR2CSVHandlerFunc)&src_rival_lamp);
    LR2CSVExecutor::AddHandler("#DST_RIVAL_LAMP", (LR2CSVHandlerFunc)&dst_rival_lamp);
    LR2CSVExecutor::AddHandler("#BAR_CENTER", (LR2CSVHandlerFunc)&bar_center);
  }
};

// register handlers
LR2CSVMusicWheelHandlers _LR2CSVMusicWheelHandlers;

}