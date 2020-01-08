#include "Song.h"
#include "Player.h"
#include "Event.h"
#include "Logger.h"
#include "Util.h"
#include "rparser.h"
#include <map>

#include <sqlite3.h>


namespace rhythmus
{

using SongAuto = std::shared_ptr<rparser::Song>;

struct SongInvalidateData
{
  std::string songpath;
  int modified_date;
  int hit_count;
};

const char* const sGamemode[] = {
  "none",
  "4key",
  "5key",
  "6key",
  "7key",
  "8key",
  "IIDXSP",
  "IIDXDP",
  "IIDX5key",
  "IIDX10key",
  "popn",
  "ez2dj",
  "ddr",
  0
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

const char* GamemodeToString(int v)
{
  if (v >= Gamemode::kGamemodeEnd)
    return nullptr;
  return sGamemode[v];
}

int StringToGamemode(const char* s)
{
  auto ss = sGamemode;
  while (*ss)
  {
    if (stricmp(s, *ss) == 0)
      return ss - sGamemode;
    ++ss;
  }
  return Gamemode::kGamemodeNone;
}

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
  virtual void run()
  {
    SongList::getInstance().LoadInvalidationList();
  }

  virtual void abort()
  {
    SongList::getInstance().ClearInvalidationList();
  }
};

// ----------------------------- class SongList

SongList::SongList()
  : is_loaded_(false)
{
  song_dir_ = "../songs";
  Clear();
}

void SongList::Load()
{
  Clear();
  is_loaded_ = false;

  /* (path, timestamp) key */
  std::map<std::string, int> mod_dir;
  std::map<std::string, int> mod_db;
  std::vector<SongInvalidateData> songcheck;
  std::vector<DirItem> dir;

  // 1. attempt to read file/folder list in directory
  if (!GetDirectoryItems(song_dir_, dir))
  {
    std::cerr <<
      "SongList: Failed to generate songlist. make sure song library path exists." <<
      std::endl;
    return;
  }
  for (auto& d : dir)
  {
    SongInvalidateData s;
    s.songpath = song_dir_ + "/" + d.filename;
    s.modified_date = d.timestamp_modified;
    s.hit_count = 0;
    songcheck.push_back(s);
  }

  // 2. attempt to load DB
  sqlite3 *db = 0;
  int rc = sqlite3_open("../system/song.db", &db);
  if (rc) {
    std::cout << "Cannot open song database, regarding as database file is missing." << std::endl;
  } else
  {
    // Load all previously loaded songs
    char *errmsg;
    sqlite3_exec(db,
      "SELECT id, title, subtitle, artist, subartist, genre, "
      "songpath, chartpath, type, level, judgediff, modified_date, "
      "notecount, length_ms, bpm_max, bpm_min, is_longnote, is_backspin "
      "from songs;",
      &SongList::sql_songlist_callback, this, &errmsg);
    if (rc != SQLITE_OK)
    {
      std::cerr << "Failed to query song database, maybe corrupted? (" << errmsg << ")" << std::endl;
      sqlite3_free(errmsg);
    }
    sqlite3_close(db);

    // mark hit count to check a song should be loaded
    // - also check current song is invalid or not.
    //   if invalid, it will removed from array...
    std::vector<SongListData> songs_new;
    for (auto& s : songs_)
    {
      int marked = 0;
      for (auto &check : songcheck)
      {
        if (s.songpath == check.songpath && s.modified_date == check.modified_date)
        {
          marked++;
          check.hit_count++;
          break;
        }
      }
      if (marked > 0)
        songs_new.push_back(s);
    }
    songs_.swap(songs_new);
  }

  // from now,
  // * songs_ : contains all confirmed chart lists (don't need to be reloaded)
  // * songcheck : hit_count == 0 if song is new, which means need to be (re)loaded.

  // 3. Now prepare invalidate song list and load with worker thread
  for (auto &check : songcheck)
  {
    if (check.hit_count == 0)
      invalidate_list_.push_back(check.songpath);
  }
  total_inval_size_ = invalidate_list_.size();
  EventManager::SendEvent("SongListLoaded");

  size_t thread_count_ = TaskPool::getInstance().GetPoolSize();
  for (size_t i = 0; i < thread_count_; ++i)
  {
    TaskAuto t = std::make_unique<SongListUpdateTask>();
    TaskPool::getInstance().EnqueueTask(t);
  }
}

int SongList::sql_songlist_callback(void *_self, int argc, char **argv, char **colnames)
{
  SongList *self = static_cast<SongList*>(_self);
  for (int i = 0; i < argc; ++i)
  {
    SongListData sdata;
    sdata.id = argv[0];
    sdata.title = argv[1];
    sdata.subtitle = argv[2];
    sdata.artist = argv[3];
    sdata.subartist = argv[4];
    sdata.genre = argv[5];
    sdata.songpath = argv[6];
    sdata.chartpath = argv[7];
    sdata.type = atoi(argv[8]);
    sdata.level = atoi(argv[9]);
    sdata.judgediff = atoi(argv[10]);
    sdata.modified_date = atoi(argv[11]);
    sdata.notecount = atoi(argv[12]);
    sdata.length_ms = atoi(argv[13]);
    sdata.bpm_max = atoi(argv[14]);
    sdata.bpm_min = atoi(argv[15]);
    sdata.is_longnote = atoi(argv[16]);
    sdata.is_backspin = atoi(argv[17]);
    self->songs_.push_back(sdata);
  }
  return 0;
}

void SongList::Save()
{
  sqlite3 *db = 0;
  int rc = sqlite3_open("../system/song.db", &db);
  if (rc) {
    std::cout << "Cannot save song database." << std::endl;
  }
  else
  {
    // Delete all records and rewrite
    char *errmsg;
    sqlite3_exec(db, "DROP TABLE songs;",
      &SongList::sql_dummy_callback, this, &errmsg);
    if (rc != SQLITE_OK)
    {
      // drop table might be failed, ignore.
      sqlite3_free(errmsg);
    }
    sqlite3_exec(db, "CREATE TABLE songs("
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
      std::cerr << "Failed SQL: " << errmsg << ")" << std::endl;
      sqlite3_free(errmsg);
      sqlite3_close(db);
      return;
    }

    for (auto &s : songs_)
    {
      std::string sql = format_string(
        "INSERT INTO songs VALUES ("
        "'%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s',"
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d"
        ");",
        s.id, s.title, s.subtitle, s.artist, s.subartist, s.genre,
        s.songpath, s.chartpath, s.type, s.level, s.judgediff, s.modified_date,
        s.notecount, s.length_ms, s.bpm_max, s.bpm_min, s.is_longnote, s.is_backspin
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
  songs_.clear();
  invalidate_list_.clear();
}

void SongList::LoadInvalidationList()
{
  std::string filepath;
  std::string chartname;

  while (true)
  {
    {
      // get song name safely.
      // if empty, then exit loop.
      std::lock_guard<std::mutex> lock(invalidate_list_mutex_);
      if (invalidate_list_.empty())
      {
        break;
      }
      std::string path = invalidate_list_.front();
      invalidate_list_.pop_front();
      current_loading_file_ = path;
      // separate songpath and chartname, if necessary.
      if (path.find('|') != std::string::npos)
      {
        Split(path, '|', filepath, chartname);
      }
      else
      {
        filepath = path;
      }
    }

    // attempt song loading.
    SongAuto song = std::make_shared<rparser::Song>();
    if (!song->Open(filepath))
    {
      load_count_++;
      break;
    }
    SongListData dat;
    dat.songpath = filepath;
    for (int i = 0; i < song->GetChartCount(); ++i)
    {
      rparser::Chart* c = nullptr;
      if (chartname.empty())
      {
        c = song->GetChart(i);
      }
      else
      {
        c = song->GetChart(chartname);
        if (!c) break;
        i = INT_MAX; // kind of trick to exit for loop instantly
      }
      c->Invalidate();
      auto &meta = c->GetMetaData();
      dat.chartpath = c->GetFilename();
      dat.id = c->GetHash();
      // TODO: automatically extract subtitle from title
      dat.title = meta.title;
      dat.subtitle = meta.subtitle;
      dat.artist = meta.artist;
      dat.subartist = meta.subartist;
      dat.genre = meta.genre;
      switch (c->GetChartType())
      {
      case rparser::CHARTTYPE::IIDXSP:
        dat.type = Gamemode::kGamemodeIIDXSP;
        break;
      case rparser::CHARTTYPE::IIDXDP:
        dat.type = Gamemode::kGamemodeIIDXDP;
        break;
      case rparser::CHARTTYPE::Popn:
        dat.type = Gamemode::kGamemodePopn;
        break;
      default:
        dat.type = Gamemode::kGamemodeNone;
      }
      switch (meta.difficulty)
      {
      case 2:
        dat.difficulty = Difficulty::kDifficultyNormal;
        break;
      case 3:
        dat.difficulty = Difficulty::kDifficultyHard;
        break;
      case 4:
        dat.difficulty = Difficulty::kDifficultyEx;
        break;
      case 5:
        dat.difficulty = Difficulty::kDifficultyInsane;
        break;
      case 0: /* XXX: what is the exact meaning of DIFF 0? */
      case 1:
      default:
        dat.difficulty = Difficulty::kDifficultyEasy;
        break;
      }
      dat.level = meta.level;
      dat.judgediff = meta.judgerank;
      dat.modified_date = 0; // TODO
      dat.notecount = static_cast<int>(c->GetScoreableNoteCount());
      dat.length_ms = static_cast<int>(c->GetSongLastObjectTime() * 1000);
      dat.is_longnote = c->HasLongnote();
      dat.is_backspin = 0; // TODO
      dat.bpm_max = c->GetTimingSegmentData().GetMaxBpm();
      dat.bpm_min = c->GetTimingSegmentData().GetMinBpm();

      {
        std::lock_guard<std::mutex> lock(songlist_mutex_);
        songs_.push_back(dat);
      }
    }

    load_count_++;
  }

  // check if invalidate list is done.
  // if so, mark as all loaded
  if (invalidate_list_.empty())
  {
    is_loaded_ = true;
  }
}

void SongList::ClearInvalidationList()
{
  std::lock_guard<std::mutex> lock(invalidate_list_mutex_);
  invalidate_list_.clear();
}

void SongList::LoadFileIntoSongList(const std::string& songpath, const std::string& chartname)
{
  // check a file is already exists
  for (auto& song : songs_)
  {
    if (song.songpath == songpath)
    {
      if (chartname.empty() || song.chartpath == chartname)
        return; /* chart already exists */
    }
  }

  // make load task
  std::string path = songpath;
  if (!chartname.empty())
    path += "|" + chartname;
  invalidate_list_.push_back(path);
  LoadInvalidationList();
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

SongList& SongList::getInstance()
{
  static SongList s;
  return s;
}

size_t SongList::size() { return songs_.size(); }
std::vector<SongListData>::iterator SongList::begin() { return songs_.begin(); }
std::vector<SongListData>::iterator SongList::end() { return songs_.end(); }
const SongListData& SongList::get(int i) const { return songs_[i]; }
SongListData SongList::get(int i) { return songs_[i]; }

}