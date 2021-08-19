#include "LR2MusicWheel.h"
#include "Song.h"
#include "SceneManager.h"
#include "ScriptLR2.h"
#include "Player.h"
#include "Setting.h"
#include "Text.h"
#include "Number.h"
#include "rparser.h"
#include "common.h"


static constexpr int NUM_BG_TYPES = 10;
static constexpr int NUM_LEVEL_TYPES = 7;
static constexpr int NUM_BAR_COUNT = 30;
static constexpr double SCROLL_TIME = 500;

unsigned _wheel_index = 0;
static std::string _GenerateNewId()
{
  static char buffer[10];
  sprintf(buffer, "%04u", _wheel_index);
  _wheel_index++;
  return buffer;
}

namespace rhythmus
{

enum class LR2WheelItemType
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

struct LR2WheelData {
  std::string id;
  LR2WheelItemType type;
  std::string title;
  std::string artist;
  int level;
  int diff;
  int clear;
  float rate;
  const ChartMetaData* chart;

  LR2WheelData();
  static LR2WheelData* CreateFromChart(const ChartMetaData* c);
  static LR2WheelData* CreateAsFolder(const std::string &name);
};

class LR2WheelItem : public BaseObject {
public:
  LR2WheelItem();
  void SetData(LR2WheelData* data);

  Sprite* GetBackground(unsigned i);
  Text* GetText();
  Number* GetLevel(unsigned i);

private:
  LR2WheelData* data_;
  Sprite bg_[10];
  Number level_[NUM_LEVEL_TYPES];
  Text title_;
  virtual void doUpdate(double delta);
  virtual void doRender();
};


// ----------------------------- LR2WheelData

LR2WheelData::LR2WheelData() :
  type(LR2WheelItemType::None), level(0), diff(0), clear(0), rate(.0f), chart(nullptr)
{}

LR2WheelData* LR2WheelData::CreateFromChart(const ChartMetaData* c)
{
  LR2WheelData* d = new LR2WheelData();
  d->id = _GenerateNewId();
  d->chart = c;
  d->type = LR2WheelItemType::Song;
  d->level = c->level;
  d->diff = c->difficulty;
  d->clear = ClearTypes::kClearNone;
  d->rate = 0;  // TODO

  d->title = c->title;
  d->artist = c->artist;

  return d;
}

LR2WheelData* LR2WheelData::CreateAsFolder(const std::string& name)
{
  LR2WheelData* d = new LR2WheelData();
  d->id = _GenerateNewId() + "F";
  d->chart = nullptr;
  d->type = LR2WheelItemType::Folder;
  d->level = 0;
  d->diff = 0;
  d->clear = ClearTypes::kClearNone;
  d->rate = 0;
  
  d->title = name;

  return d;
}


// ----------------------------- MusicWheelItem

LR2WheelItem::LR2WheelItem() : data_(nullptr)
{
  for (size_t i = 0; i < NUM_BG_TYPES; ++i)
    AddStaticChild(&bg_[i]);
  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
    AddStaticChild(&level_[i]);
  AddStaticChild(&title_);
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

void LR2WheelItem::SetData(LR2WheelData* d)
{
  const unsigned typeidx = (unsigned)d->type;

  // if data is nullptr, then clear all data and set as empty item
  if (d == nullptr) {
    title_.Hide();
    for (size_t i = 1; i < NUM_BG_TYPES; ++i)
      bg_[i].Hide();
    for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i)
      level_[i].Hide();
    bg_[0].Hide();
    return;
  }

  title_.SetText(d->title);

  for (size_t i = 0; i < NUM_BG_TYPES; ++i)
    bg_[i].Hide();
  bg_[_type_to_baridx[typeidx]].Show();

  for (size_t i = 0; i < NUM_LEVEL_TYPES; ++i) {
    if (i == d->diff && _type_to_disp_level[typeidx]) {
      level_[i].Show();
      level_[i].SetNumber(d->level);
    }
    else {
      level_[i].Hide();
      // XXX: kind of trick
      // LR0 event causes level in not hidden state,
      // so make it empty string instead of hidden.
      level_[i].SetText(std::string());
    }
  }
}

Sprite* LR2WheelItem::GetBackground(unsigned type) { return &bg_[type]; }
Number* LR2WheelItem::GetLevel(unsigned type) { return &level_[type]; }
Text* LR2WheelItem::GetText() { return &title_; }

void LR2WheelItem::doUpdate(double delta)
{
  float w, h;
  BaseObject::doUpdate(delta);
  w = rhythmus::GetWidth(frame_.pos);
  h = rhythmus::GetHeight(frame_.pos);
  for (size_t i = 0; i < NUM_BG_TYPES; ++i) {
    bg_[i].SetWidth(w);
    bg_[i].SetHeight(h);
  }
}

void LR2WheelItem::doRender()
{
  BaseObject::doRender();

  // XXX: Debug code
  //GRAPHIC->DrawRect(0, 0, 200, 60, 0x66FF9999);
}

// --------------------------- class MusicWheel

LR2MusicWheel::LR2MusicWheel() :
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
  sort_ = Sorttype::kNoSort;
  filter_.gamemode = 0;
  filter_.difficulty = 0;

  invalidate_data_ = false;
  bar_center_ = 10;
  data_index_ = 0;
  scroll_time_ = 0;
  scroll_ = 0;

  /* create section datas (TODO) */
  data_sections_.push_back(
    LR2WheelData::CreateAsFolder("All Songs")
  );
  data_sections_.push_back(
    LR2WheelData::CreateAsFolder("New Songs")
  );

  /* create items and add to child */
  for (unsigned i = 0; i < NUM_BAR_COUNT; ++i) {
    auto* item = new LR2WheelItem();
    items_.push_back(item);
    AddStaticChild(item);
  }
  for (unsigned i = 0; i < NUM_BAR_COUNT; ++i) {
    auto* p = new BaseObject();
    pos_on_.push_back(p);
    AddStaticChild(p);
  }
  for (unsigned i = 0; i < NUM_BAR_COUNT; ++i) {
    auto* p = new BaseObject();
    pos_off_.push_back(p);
    AddStaticChild(p);
  }
}

LR2MusicWheel::~LR2MusicWheel()
{
  ClearData();
  for (auto* p : data_sections_) delete p;
  for (auto* p : pos_on_) delete p;
  for (auto* p : pos_off_) delete p;
  for (auto* p : items_) delete p;
}

void LR2MusicWheel::OnReady()
{
  /* load system preference */
  filter_.gamemode = PrefValue<int>("gamemode").get();
  filter_.difficulty = PrefValue<int>("difficulty").get();
  filter_.key = 0;
  sort_ = PrefValue<int>("sortmode").get();
  invalidate_data_ = true;

  Wheel::OnReady();

  RebuildItem();
}

void LR2MusicWheel::OnSelectChange(const void* _, int direction)
{
  const LR2WheelData* data = (LR2WheelData*)_;
  if (data == nullptr) {
    static LR2WheelData data_empty;
    data = &data_empty;
  }

  // update KeyPool
  const auto* songinfo = data->chart;
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
  *info_itemtype = (int)data->type;


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
  auto *playrecord = PlayerManager::GetPlayer()->GetPlayRecord(data->id);
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
  *info_musicwheelpos = static_cast<float>(
    data_index_ * 100.0 / data_.size()
    );

  // send event
  EVENTMAN->SendEvent("SongSelectChanged");
  if (direction == -1)
    EVENTMAN->SendEvent("SongSelectChangeUp");
  else if (direction == 1)
    EVENTMAN->SendEvent("SongSelectChangeDown");
}

void LR2MusicWheel::OnSelectChanged()
{
  EVENTMAN->SendEvent("SongSelectChanged");
}

void LR2MusicWheel::MoveDown()
{
  scroll_time_ = SCROLL_TIME;
  scroll_ = 1;
  if (data_.size() > 0)
    data_index_ = (data_index_ + 1) % data_.size();
}

void LR2MusicWheel::MoveUp()
{
  scroll_time_ = SCROLL_TIME;
  scroll_ = -1;
  if (data_.size() > 0) {
    data_index_ = (data_index_ == 0)
      ? (data_.size() - 1)
      : (data_index_ - 1);
  }
}

void LR2MusicWheel::Expand()
{
  if (data_.empty()) return;
  auto* d = data_[data_index_];

  if (d->type == LR2WheelItemType::Folder) {
    if (d->id == current_section_)
      Collapse();
    else {
      current_section_ = d->id;
      RebuildItem();
    }
  }
  else if (d->type == LR2WheelItemType::Song) {
    NextDifficultyFilter();
  }
}

void LR2MusicWheel::Collapse()
{
  if (!current_section_.empty()) {
    SetSelectedItemId(current_section_);
    current_section_.clear();
    RebuildItem();
  }
}

const std::string& LR2MusicWheel::GetSelectedItemId() const
{
  R_ASSERT(!data_.empty());
  return data_[data_index_]->id;
}

void LR2MusicWheel::SetSelectedItemId(const std::string& id)
{
  /**
   * @warn
   * If you changed selected index from external method,
   * then you should call RebuildItem() manually to refresh item.
   */
  for (unsigned i = 0; i < data_.size(); ++i) {
    if (data_[i]->id == id) {
      data_index_ = i;
      break;
    }
  }
}

void LR2MusicWheel::RebuildItem()
{
  if (invalidate_data_) {
    // filter songs -- invalidates chart data
    std::vector<const SongMetaData*> songlist;
    std::vector<const ChartMetaData*> chartlist;
    std::vector<LR2WheelData*> data_new;

    // XXX: need to send in sort invalidation
    KEYPOOL->GetInt("difficulty").set(filter_.difficulty);
    KEYPOOL->GetInt("gamemode").set(filter_.gamemode);
    KEYPOOL->GetInt("sortmode").set(sort_);
    EVENTMAN->SendEvent("SongFilterChanged");

    // clear previous data
    ClearData();

    // song filtering: gamemode
    for (const auto* song : SONGLIST->GetSongList()) {
      bool pass_filter = true;

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
    // TODO: make chartlist per difficulty in SONGLIST
    for (const auto* song : songlist) {
      ChartMetaData* c = song->chart;
      LR2WheelData* prev_chart = nullptr;
      while (c) {
        bool pass_filter;
        pass_filter = (filter_.key == 0 || c->key == filter_.key);
        pass_filter &= (filter_.difficulty == Difficulty::kDifficultyNone ||
          c->difficulty == filter_.difficulty);

        if (!pass_filter)
          chartlist.push_back(c);

        c = c->next;
        if (c == song->chart) break;
      }
    }

    for (auto* c : chartlist) {
      data_new.push_back(LR2WheelData::CreateFromChart(c));
    }

    // sort data object
    switch (sort_)
    {
    case Sorttype::kNoSort:
      break;
    case Sorttype::kSortByLevel:
      std::sort(data_new.begin(), data_new.end(),
        [](const LR2WheelData* a, const LR2WheelData* b) {
          return a->level < b->level;
        });
      break;
    case Sorttype::kSortByTitle:
      std::sort(data_new.begin(), data_new.end(),
        [](const LR2WheelData* a, const LR2WheelData* b) {
          return a->title < b->title;
        });
      break;
    case Sorttype::kSortByClear:
      std::sort(data_new.begin(), data_new.end(),
        [](const LR2WheelData* a, const LR2WheelData* b) {
          return a->clear < b->clear;
        });
      break;
    case Sorttype::kSortByRate:
      std::sort(data_new.begin(), data_new.end(),
        [](const LR2WheelData* a, const LR2WheelData* b) {
          return a->rate < b->rate;
        });
      break;
    default:
      R_ASSERT(0);
    }

    for (auto* s : data_sections_) {
      data_.push_back(s);
      if (s->id == current_section_) {
        data_.insert(data_.end(), data_new.begin(), data_new.end());
      }
    }
  }

  // rebuild rendering items
  if (!data_.empty()) {
    unsigned data_index_for_item =
      static_cast<unsigned>(
      ((int)data_index_ - bar_center_ + data_.size()) % data_.size()
        );
    for (auto* item : items_) {
      item->SetData(data_[data_index_for_item]);
      data_index_for_item = (data_index_for_item + 1) % data_.size();
    }
  }
  else {
    // for exceptional logic.
    // this logic shouldn't run in general case.
    for (auto* item : items_) {
      item->SetData(nullptr);
    }
  }
}

void LR2MusicWheel::ClearData()
{
  for (auto* d : data_) {
    // only delete generated items in RebuildItem procedure.
    // (folder is default section element so don't delete it.)
    if (d->type != LR2WheelItemType::Folder)
      delete d;
  }
  data_.clear();
}

void LR2MusicWheel::SetSort(int sort)
{
  sort_ = sort;
  invalidate_data_ = true;
}

void LR2MusicWheel::SetGamemodeFilter(int filter)
{
  filter_.gamemode = filter;
  invalidate_data_ = true;
}

void LR2MusicWheel::SetDifficultyFilter(int filter)
{
  filter_.difficulty = filter;
  invalidate_data_ = true;
}

void LR2MusicWheel::NextSort()
{
  sort_ = (sort_ + 1) % Sorttype::kSortEnd;
  invalidate_data_ = true;
}

void LR2MusicWheel::NextGamemodeFilter()
{
  filter_.gamemode = (filter_.gamemode + 1) % Gamemode::kGamemodeEnd;
  invalidate_data_ = true;
}

void LR2MusicWheel::NextDifficultyFilter()
{
  filter_.difficulty = (filter_.difficulty + 1) % Difficulty::kDifficultyEnd;
  invalidate_data_ = true;
}

void LR2MusicWheel::doUpdate(double delta)
{
  // update position of wheel items.
  // TODO: change to doUpdateAfter(...)
  DrawProperty dprop;
  double scr = 0;

  Wheel::doUpdate(delta);

  // update scroll delta
  if (scroll_time_ < SCROLL_TIME) {
    scroll_time_ -= std::max(.0, scroll_time_ - delta);
    scr = scroll_time_ / SCROLL_TIME;
  }

  for (unsigned i = 1; i < items_.size() - 1; ++i) {
    if (scr == 0) {
      // not scrolling.
      dprop = pos_off_[i]->GetCurrentFrame();
    }
    else {
      if (scroll_ < 0) {
        MakeTween(dprop,
          pos_off_[i - 1]->GetCurrentFrame(),
          pos_off_[i]->GetCurrentFrame(),
          scr, EaseTypes::kEaseIn);
      }
      else if (scroll_ > 0) {
        MakeTween(dprop,
          pos_off_[i + 1]->GetCurrentFrame(),
          pos_off_[i]->GetCurrentFrame(),
          scr, EaseTypes::kEaseIn);
      }
    }
    items_[i]->SetPos(dprop.pos);
  }
}

// TODO: set song index
// if next song difficulty is merged into single item,
// then replace current data with next difficulty item.
// else, search another difficulty item and set data index with that.

// TODO: Cancel or go to next filter
// if current filter is empty.



#define HANDLERLR2_OBJNAME LR2MusicWheel
REGISTER_LR2OBJECT(LR2MusicWheel);

class LR2MusicWheelLR2Handler : public LR2FnClass {
public:
  HANDLERLR2(SRC_BAR_BODY) {
    unsigned bgtype = (unsigned)args.get_int(1);
    for (unsigned i = 0; i < o->items_.size(); ++i) {
      auto* item = o->items_[i];
      auto* bg = item->GetBackground(bgtype);
      bg->RunCommandLR2("#SRC_IMAGE", args);
    }
  }
  HANDLERLR2(DST_BAR_BODY_OFF) {
    unsigned i = (unsigned)args.get_int(1);
    auto* item = o->pos_off_[i];
    item->RunCommandLR2("#DST", args);
  }
  HANDLERLR2(DST_BAR_BODY_ON) {
    unsigned i = (unsigned)args.get_int(1);
    auto* item = o->pos_on_[i];
    item->RunCommandLR2("#DST", args);
  }
  HANDLERLR2(BAR_CENTER) {
    o->bar_center_ = args.get_int(1);
  }
  HANDLERLR2(BAR_AVAILABLE) {
    //wheel->set_item_min_index(0);
    //wheel->set_item_max_index((unsigned)ctx->get_int(1));
  }
  HANDLERLR2(SRC_BAR_TITLE) {
    for (unsigned i = 0; i < o->items_.size(); ++i) {
      auto* item = o->items_[i];
      auto* text = item->GetText();
      text->RunCommandLR2("#SRC_TEXT", args);
    }
  }
  HANDLERLR2(DST_BAR_TITLE) {
    for (unsigned i = 0; i < o->items_.size(); ++i) {
      auto* item = o->items_[i];
      auto* text = item->GetText();
      text->RunCommandLR2("#DST_TEXT", args);
      text->RunCommandLR2("#DST", args);
    }
  }
  HANDLERLR2(SRC_BAR_FLASH) {
  }
  HANDLERLR2(DST_BAR_FLASH) {
  }
  HANDLERLR2(SRC_BAR_LEVEL) {
  }
  HANDLERLR2(DST_BAR_LEVEL) {
  }
  HANDLERLR2(SRC_BAR_LAMP) {
  }
  HANDLERLR2(DST_BAR_LAMP) {
  }
  HANDLERLR2(SRC_MY_BAR_LAMP) {
  }
  HANDLERLR2(DST_MY_BAR_LAMP) {
  }
  HANDLERLR2(SRC_RIVAL_BAR_LAMP) {
  }
  HANDLERLR2(DST_RIVAL_BAR_LAMP) {
  }
  HANDLERLR2(SRC_RIVAL_LAMP) {
  }
  HANDLERLR2(DST_RIVAL_LAMP) {
  }
  LR2MusicWheelLR2Handler() : LR2FnClass(GetTypename<LR2MusicWheel>()) {
    ADDSHANDLERLR2(SRC_BAR_BODY);
    ADDSHANDLERLR2(DST_BAR_BODY_OFF);
    ADDSHANDLERLR2(DST_BAR_BODY_ON);
    ADDSHANDLERLR2(BAR_CENTER);
    ADDSHANDLERLR2(BAR_AVAILABLE);
    ADDSHANDLERLR2(SRC_BAR_TITLE);
    ADDSHANDLERLR2(DST_BAR_TITLE);
    ADDSHANDLERLR2(SRC_BAR_FLASH);
    ADDSHANDLERLR2(DST_BAR_FLASH);
    ADDSHANDLERLR2(SRC_BAR_LEVEL);
    ADDSHANDLERLR2(DST_BAR_LEVEL);
    ADDSHANDLERLR2(SRC_BAR_LAMP);
    ADDSHANDLERLR2(DST_BAR_LAMP);
    ADDSHANDLERLR2(SRC_MY_BAR_LAMP);
    ADDSHANDLERLR2(DST_MY_BAR_LAMP);
    ADDSHANDLERLR2(SRC_RIVAL_BAR_LAMP);
    ADDSHANDLERLR2(DST_RIVAL_BAR_LAMP);
    ADDSHANDLERLR2(SRC_RIVAL_LAMP);
    ADDSHANDLERLR2(DST_RIVAL_LAMP);
  }
};

LR2MusicWheelLR2Handler _LR2MusicWheelLR2Handler;

}