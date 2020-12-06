#include "Song.h"
#include "Game.h"
#include "Player.h"
#include "Event.h"
#include "Logger.h"
#include "Util.h"
#include "common.h"
#include "rparser.h"

#include <sqlite3.h>


namespace rhythmus
{

using SongAuto = std::shared_ptr<rparser::Song>;

struct SongInvalidateData
{
  std::string songpath;
  int64_t modified_date = 0;
  int hit_count = 0;
};

const char* const sDifficulty[] = {
  "none",
  "beginner",
  "easy",
  "normal",
  "hard",
  "ex",
  "insane",
  0
};

const char* const sSorttype[] = {
  "none",
  "title",
  "level",
  "clear",
  "rate",
  0
};

const char* DifficultyToString(int v)
{
  if (v >= Difficulty::kDifficultyEnd)
    return nullptr;
  return sDifficulty[v];
}

int StringToDifficulty(const char* s)
{
  auto ss = sDifficulty;
  while (*ss)
  {
    if (stricmp(s, *ss) == 0)
      return ss - sDifficulty;
    ++ss;
  }
  return Difficulty::kDifficultyNone;
}

const char* SorttypeToString(int v)
{
  if (v >= Sorttype::kSortEnd)
    return nullptr;
  return sSorttype[v];
}

int StringToSorttype(const char* s)
{
  auto ss = sSorttype;
  while (*ss)
  {
    if (stricmp(s, *ss) == 0)
      return ss - sSorttype;
    ++ss;
  }
  return Sorttype::kNoSort;
}

// ------------------- class SongListUpdateTask

class SongListUpdateTask : public Task
{
public:
  SongListUpdateTask(const std::string &path) :
    fullpath_(path), is_aborted_(false)
  {
    // separate songpath and chartname, if necessary.
    if (path.find('|') != std::string::npos)
    {
      Split(path, '|', filepath_, chartname_);
    }
    else
    {
      filepath_ = path;
    }
  }

  virtual void run()
  {
    std::vector<ChartMetaData*> cdat_list;
    ChartMetaData *cdat, *cdat_prev;
    SongMetaData* sdat;

    cdat = cdat_prev = nullptr;
    sdat = nullptr;

    // check is aborted..?
    if (is_aborted_)
      return;

    // attempt song loading.
    SongAuto song = std::make_shared<rparser::Song>();
    if (!song->Open(filepath_))
    {
      // Actually this is not a song that this program can read,
      // but is necessary to be cached to prevent parsing it next time again.
      // So don't return here, call PushChart(..) with unknown type,
      // and invoke FinishSongLoading().
      Logger::Error("Songlist read failure: %s", fullpath_.c_str());

      // TODO: call PushChart(..) with unknown type
    }
    else
    {
      SONGLIST->StartSongLoading(filepath_);
      sdat = new SongMetaData();
      sdat->modified_time = 0;
      sdat->count = 0;
      sdat->chart = nullptr;
      sdat->path = filepath_;
      sdat->type = Gamemode::kGamemodeNone;

      for (unsigned i = 0; i < song->GetChartCount(); ++i)
      {
        rparser::Chart* c = nullptr;
        int type;
        int difficulty;

        cdat = new ChartMetaData();
        if (chartname_.empty()) {
          c = song->GetChart(i);
        }
        else {
          c = song->GetChart(chartname_);
          if (!c) break;
          i = INT_MAX; // kind of trick to exit for loop instantly
        }
        c->Update();
        auto &meta = c->GetMetaData();
        cdat->songpath = filepath_;
        cdat->chartpath = c->GetFilename();
        cdat->id = c->GetHash();
        // TODO: automatically extract subtitle from title
        cdat->title = meta.title;
        cdat->subtitle = meta.subtitle;
        cdat->artist = meta.artist;
        cdat->subartist = meta.subartist;
        cdat->genre = meta.genre;
        switch (c->GetChartType())
        {
        case rparser::CHARTTYPE::IIDXSP:
        case rparser::CHARTTYPE::IIDXDP:
          type = Gamemode::kGamemodeIIDX;
          break;
        case rparser::CHARTTYPE::Popn:
          type = Gamemode::kGamemodePopn;
          break;
        default:
          type = Gamemode::kGamemodeNone;
        }
        switch (meta.difficulty)
        {
        case 2:
          difficulty = Difficulty::kDifficultyNormal;
          break;
        case 3:
          difficulty = Difficulty::kDifficultyHard;
          break;
        case 4:
          difficulty = Difficulty::kDifficultyEx;
          break;
        case 5:
          difficulty = Difficulty::kDifficultyInsane;
          break;
        case 0: /* XXX: what is the exact meaning of DIFF 0? */
        case 1:
        default:
          difficulty = Difficulty::kDifficultyEasy;
          break;
        }
        cdat->type = type;
        cdat->difficulty = difficulty;
        cdat->level = meta.level;
        cdat->judgediff = meta.difficulty;
        cdat->modified_date = 0; // TODO
        cdat->notecount = static_cast<int>(c->GetScoreableNoteCount());
        cdat->length_ms = static_cast<int>(c->GetSongLastObjectTime() * 1000);
        cdat->is_longnote = c->HasLongnote();
        cdat->is_backspin = 0; // TODO
        cdat->bpm_max = (int)c->GetTimingSegmentData().GetMaxBpm();
        cdat->bpm_min = (int)c->GetTimingSegmentData().GetMinBpm();

        cdat_list.push_back(cdat);
        sdat->count++;
      }

      // make linked-list between charts by ascending.
      // TODO: sort by difficulty/level
      if (sdat->count > 0) {
        cdat_prev = cdat_list.back();
        for (auto* c : cdat_list) {
          c->prev = cdat_prev;
          cdat_prev->next = c;
          if (!SONGLIST->PushChart(c)) {
            cdat_prev->next = cdat_prev;
            sdat->count--;
            delete c;
            continue;
          }
          cdat_prev = c;
        }
      }

      // append song data into songlist
      if (sdat->count > 0) {
        cdat_list.back()->next = cdat_list.front();
        sdat->chart = cdat_list.front();
        sdat->type = sdat->chart->type;
        SONGLIST->PushSong(sdat);
      }
      else delete sdat;
    }

    SONGLIST->FinishSongLoading();
  }

  virtual void abort()
  {
    is_aborted_ = true;
  }

private:
  std::string fullpath_;
  std::string filepath_;
  std::string chartname_;
  bool is_aborted_;
};

// ----------------------------- class SongList

SongList::SongList()
  : is_loaded_(false)
{
  song_dir_ = "./songs";
  song_db_ = "./system/song.db";
  Clear();
}

SongList::~SongList() { Clear(); }

void SongList::Initialize()
{
  R_ASSERT(SONGLIST == 0);
  SONGLIST = new SongList();
}

void SongList::Cleanup()
{
  delete SONGLIST;
  SONGLIST = nullptr;
}

bool SongList::LoadFromDatabase(const std::string& path)
{
  sqlite3* db = 0;
  char* errmsg = nullptr;
  int rc = sqlite3_open(song_db_.c_str(), &db);
  if (rc) {
    Logger::Warn("Cannot open song database, regarding as database file is missing.");
    return false;
  }
  else {
    // Load all cached songs
    rc = sqlite3_exec(db,
      "SELECT path, type, modified_date "
      "from songs;",
      &SongList::sql_songlist_callback, this, &errmsg);
    if (rc != SQLITE_OK) {
      Logger::Error("Failed to query song table from database, maybe corrupted? (%s)", errmsg);
      sqlite3_free(errmsg);
      sqlite3_close(db);
      return false;
    }

    // Load all cached charts
    rc = sqlite3_exec(db,
      "SELECT id, title, subtitle, artist, subartist, genre, "
      "songpath, chartpath, type, level, judgediff, modified_date, "
      "notecount, length_ms, bpm_max, bpm_min, is_longnote, is_backspin "
      "from charts;",
      &SongList::sql_chartlist_callback, this, &errmsg);
    if (rc != SQLITE_OK) {
      Logger::Error("Failed to query chart table from database, maybe corrupted? (%s)", errmsg);
      sqlite3_free(errmsg);
      sqlite3_close(db);
      return false;
    }
    sqlite3_close(db);
  }

  // Link chart to song (fill in-memory attributes)
  for (auto* c : charts_) {
    auto* song = FindSong(c->songpath);
    if (!song) {
      Logger::Error("Failed to find songs matching with chart %s (%s)",
                    c->chartpath.c_str(), c->songpath.c_str());
      continue;
    }
    c->song = song;
    if (song->chart == nullptr) song->chart = c;
    song->count++;
  }

  return true;
}

bool SongList::CreateDatabase(sqlite3 *db)
{
  // Delete all records and tables
  char* errmsg;
  int rc;
  rc = sqlite3_exec(db, "DROP TABLE charts;",
    &SongList::sql_dummy_callback, this, &errmsg);
  if (rc != SQLITE_OK)
  {
    // drop table might be failed, ignore.
    sqlite3_free(errmsg);
  }

  rc = sqlite3_exec(db, "DROP TABLE songs;",
    &SongList::sql_dummy_callback, this, &errmsg);
  if (rc != SQLITE_OK)
  {
    // drop table might be failed, ignore.
    sqlite3_free(errmsg);
  }

  // Create tables
  sqlite3_exec(db, "CREATE TABLE songs("
    "path CHAR(128) PRIMARY KEY,"
    "type INT,"
    "modified_date INT"
    ");",
    &SongList::sql_dummy_callback, this, &errmsg);
  if (rc != SQLITE_OK)
  {
    Logger::Error("Failed SQL: %s", errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(db);
    return false;
  }
  sqlite3_exec(db, "CREATE TABLE charts("
    "id CHAR(128) PRIMARY KEY,"
    "title CHAR(128),"
    "subtitle CHAR(128),"
    "artist CHAR(128),"
    "subartist CHAR(128),"
    "genre CHAR(64),"
    "songpath CHAR(1024) NOT NULL,"
    "chartpath CHAR(512) NOT NULL,"
    "type INT,"
    "level INT,"
    "judgediff INT,"
    "modified_date INT,"
    "notecount INT,"
    "length_ms INT,"
    "bpm_max INT,"
    "bpm_min INT,"
    "is_longnote INT,"
    "is_backspin INT"
    ");",
    &SongList::sql_dummy_callback, this, &errmsg);
  if (rc != SQLITE_OK)
  {
    Logger::Error("Failed SQL: %s", errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(db);
    return false;
  }

  return true;
}

void SongList::Load()
{
  Clear();
  is_loaded_ = true;    /* consider all song is loaded in initial state. */

  // attempt to load DB
  LoadFromDatabase(song_db_);

  // check directories for new/deleted songs
  Update();
}

/**
 * @brief
 * Updates songlist without reloading database.
 * Use cached songlist and chartlist in memory.
 */
void SongList::Update()
{
  /* (path, timestamp) key */
  std::map<std::string, int> mod_dir;
  std::map<std::string, int> mod_db;
  std::vector<SongInvalidateData> songcheck;
  std::vector<DirItem> dir;
  std::vector<Task*> tasklist;
  std::vector<ChartMetaData*> charts_valid, charts_invalid;
  std::vector<SongMetaData*> songs_valid;
  ChartMetaData* curr_chart = nullptr;

  // 1. attempt to read file/folder list in directory
  if (!GetDirectoryItems(song_dir_, dir)) {
    Logger::Error("SongList: Failed to generate songlist. make sure song library path exists.");
    return;
  }
  for (auto& d : dir) {
    SongInvalidateData s;
    s.songpath = song_dir_ + "/" + d.filename;
    s.modified_date = d.timestamp_modified;
    s.hit_count = 0;
    songcheck.push_back(s);
  }

  // 2.
  // mark hit count for cached charts to check a song is a new one.
  // and also check if chart is valid(not deleted) one.
  // if invalid, it should be removed from array.
  for (auto* c : charts_) {
    int marked = 0;
    for (auto &check : songcheck) {
      if (c->songpath == check.songpath &&
          c->modified_date == check.modified_date) {
        marked++;
        check.hit_count++;
        break;
      }
    }
    if (marked > 0)
      charts_valid.push_back(c);
    else
      charts_invalid.push_back(c);
    curr_chart = c;
  }

  for (auto* c : charts_invalid) {
    c->next->prev = c->prev;
    c->prev->next = c->next;
    if (c->song->chart == c) {
      c->song->chart = c->next;
    }
    c->song->count--;
    delete c;
  }

  for (auto* s : songs_) {
    if (s->count == 0)
      delete s;
    else
      songs_valid.push_back(s);
  }

  Logger::Info("Check for cached charts: Valid [%u], Invalid [%u]",
    charts_valid.size(), charts_invalid.size());
  Logger::Info("New song directories found [%u]", songcheck.size());

  charts_.swap(charts_valid);
  songs_.swap(songs_valid);

  // from now,
  // * charts_ : contains all confirmed chart lists (don't need to be reloaded)
  // * songcheck : hit_count == 0 if song is new, which means need to be (re)loaded.

  // 3. Now create all invalidate song list into Task
  is_loaded_ = true;
  for (auto &check : songcheck) {
    if (check.hit_count == 0) {
      Task* t = new SongListUpdateTask(check.songpath);
      tasklist.push_back(t);
      total_inval_size_++;
      is_loaded_ = false;   /* Song is not loaded yet in this state! */
    }
  }
  for (auto *t : tasklist)
    TaskPool::getInstance().EnqueueTask(t);

  Logger::Info("Songlist reload status: File found(song) %u, "
               "Cache found(chart) %u, Validate(song) %u",
    dir.size(), songs_.size(), total_inval_size_);
}

int SongList::sql_songlist_callback(void* _self, int argc, char** argv, char** colnames)
{
  SongList* self = static_cast<SongList*>(_self);
  SongMetaData* song = new SongMetaData();

  song->path = argv[0];
  song->type = 0;
  song->modified_time = atoi(argv[1]);
  song->count = 0;
  song->chart = nullptr;

  self->PushSong(song);
  return true;
}

int SongList::sql_chartlist_callback(void *_self, int argc, char **argv, char **colnames)
{
  SongList *self = static_cast<SongList*>(_self);
  ChartMetaData *chart = new ChartMetaData();

  chart->id = argv[0];
  chart->title = argv[1];
  chart->subtitle = argv[2];
  chart->artist = argv[3];
  chart->subartist = argv[4];
  chart->genre = argv[5];
  chart->songpath = argv[6];
  chart->chartpath = argv[7];
  chart->type = atoi(argv[8]);
  chart->level = atoi(argv[9]);
  chart->judgediff = atoi(argv[10]);
  chart->modified_date = atoi(argv[11]);
  chart->notecount = atoi(argv[12]);
  chart->length_ms = atoi(argv[13]);
  chart->bpm_max = atoi(argv[14]);
  chart->bpm_min = atoi(argv[15]);
  chart->is_longnote = atoi(argv[16]);
  chart->is_backspin = atoi(argv[17]);
  chart->song = nullptr;
  chart->next = chart->prev = nullptr;

  if (self->PushChart(chart))
    return true;

  // if failed
  delete chart;
  return false;
}

void SongList::Save()
{
  sqlite3 *db = 0;
  char* errmsg;
  int rc = sqlite3_open(song_db_.c_str(), &db);
  if (rc) {
    std::cout << "Cannot save song database." << std::endl;
  }
  else
  {
    CreateDatabase(db);
    for (const auto* s : songs_)
    {
      std::string sql = format_string(
        "INSERT INTO songs VALUES ("
        "'%s', %d"
        ");",
        s->path.c_str(), s->modified_time
      );
      sqlite3_exec(db, sql.c_str(),
        &SongList::sql_dummy_callback, this, &errmsg);
      if (rc != SQLITE_OK)
      {
        std::cerr << "Failed SQL: " << errmsg << ")" << std::endl;
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return;
      }
    }
    for (const auto *s : charts_)
    {
      std::string sql = format_string(
        "INSERT INTO charts VALUES ("
        "'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s',"
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d"
        ");",
        s->id.c_str(), s->title.c_str(), s->subtitle.c_str(), s->artist.c_str(), s->subartist.c_str(),
        s->genre.c_str(),
        s->songpath.c_str(), s->chartpath.c_str(), s->type, s->level, s->judgediff, s->modified_date,
        s->notecount, s->length_ms, s->bpm_max, s->bpm_min, s->is_longnote, s->is_backspin
      );
      sqlite3_exec(db, sql.c_str(),
        &SongList::sql_dummy_callback, this, &errmsg);
      if (rc != SQLITE_OK)
      {
        std::cerr << "Failed SQL: " << errmsg << ")" << std::endl;
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return;
      }
    }
    sqlite3_close(db);
  }
}

void SongList::Clear()
{
  total_inval_size_ = 0;
  load_count_ = 0;
  for (auto* c : charts_)
    delete c;
  charts_.clear();
  songs_.clear();
}

void SongList::LoadFileIntoChartList(const std::string& songpath, const std::string& chartname)
{
  // check a file is already exists
  for (const auto *c : charts_)
  {
    if (c->songpath == songpath)
    {
      if (chartname.empty() || c->chartpath == chartname)
        return; /* chart already exists */
    }
  }

  // make load task
  std::string path = songpath;
  if (!chartname.empty())
    path += "|" + chartname;
  is_loaded_ = false;
  Task* t = new SongListUpdateTask(path);
  TaskPool::getInstance().EnqueueTask(t);
}

int SongList::sql_dummy_callback(void*, int argc, char **argv, char **colnames)
{
  return 0;
}

double SongList::get_progress() const
{
  if (total_inval_size_ == 0)
    return 1.0;
  else
    return (double)load_count_ / total_inval_size_;
}

bool SongList::is_loaded() const
{
  return is_loaded_;
}

std::string SongList::get_loading_filename() const
{
  return current_loading_file_;
}

size_t SongList::song_count() const { return songs_.size(); }
size_t SongList::chart_count() const { return charts_.size(); }


SongMetaData* SongList::FindSong(const std::string& path)
{
  for (auto* song : songs_)
    if (song->path == path) return song;
  return nullptr;
}

ChartMetaData* SongList::FindChart(const std::string& id)
{
  for (auto* c : charts_)
    if (c->id == id) return c;
  return nullptr;
}

const std::vector<SongMetaData*>& SongList::GetSongList() const
{
  return songs_;
}

const std::vector<ChartMetaData*>& SongList::GetChartList() const
{
  return charts_;
}

bool SongList::PushChart(ChartMetaData *p)
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  
  // check duplication
  for (const auto* chart : charts_) {
    if (chart->id == p->id &&
        chart->songpath == p->songpath &&
        chart->chartpath == p->chartpath)
    {
      Logger::Warn("Song data is already exists (%s, %s)",
        chart->songpath.c_str(), chart->chartpath.c_str());
      return false;
    }
  }

  charts_.push_back(p);
  return true;
}

void SongList::PushSong(SongMetaData* p)
{
  // Warning: this method must be called on non-duplicated song object
  songs_.push_back(p);
}

void SongList::StartSongLoading(const std::string &name)
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  current_loading_file_ = name;
}

/* @warn This function should be thread-safe */
void SongList::FinishSongLoading()
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  load_count_++;
  if (load_count_ == total_inval_size_)
  {
    is_loaded_ = true;
    Save();
    EVENTMAN->SendEvent("SongListLoaded");
  }
}


SongList *SONGLIST = nullptr;

}
