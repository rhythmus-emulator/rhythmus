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
  int64_t modified_date;
  int hit_count;
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
    SongListData dat;

    // check is aborted..?
    if (is_aborted_)
      return;

    // attempt song loading.
    SongAuto song = std::make_shared<rparser::Song>();
    if (!song->Open(filepath_))
    {
      // XXX: make status for Task ..?
      Logger::Error("Song read failure: %s", fullpath_.c_str());
      return;
    }

    SONGLIST->StartSongLoading(filepath_);

    dat.songpath = filepath_;
    for (unsigned i = 0; i < song->GetChartCount(); ++i)
    {
      rparser::Chart* c = nullptr;
      if (chartname_.empty())
      {
        c = song->GetChart(i);
      }
      else
      {
        c = song->GetChart(chartname_);
        if (!c) break;
        i = INT_MAX; // kind of trick to exit for loop instantly
      }
      c->Update();
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
      dat.judgediff = meta.difficulty;
      dat.modified_date = 0; // TODO
      dat.notecount = static_cast<int>(c->GetScoreableNoteCount());
      dat.length_ms = static_cast<int>(c->GetSongLastObjectTime() * 1000);
      dat.is_longnote = c->HasLongnote();
      dat.is_backspin = 0; // TODO
      dat.bpm_max = (int)c->GetTimingSegmentData().GetMaxBpm();
      dat.bpm_min = (int)c->GetTimingSegmentData().GetMinBpm();

      SONGLIST->PushChart(dat);
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
  song_dir_ = "../songs";
  Clear();
}

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

void SongList::Load()
{
  Clear();
  is_loaded_ = true;    /* consider all song is loaded in initial state. */

  /* (path, timestamp) key */
  std::map<std::string, int> mod_dir;
  std::map<std::string, int> mod_db;
  std::vector<SongInvalidateData> songcheck;
  std::vector<DirItem> dir;

  // 1. attempt to read file/folder list in directory
  if (!GetDirectoryItems(song_dir_, dir))
  {
    Logger::Error("SongList: Failed to generate songlist. make sure song library path exists.");
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
    Logger::Error("Cannot open song database, regarding as database file is missing.");
  } else
  {
    // Load all previously loaded songs
    char *errmsg;
    rc = sqlite3_exec(db,
      "SELECT id, title, subtitle, artist, subartist, genre, "
      "songpath, chartpath, type, level, judgediff, modified_date, "
      "notecount, length_ms, bpm_max, bpm_min, is_longnote, is_backspin "
      "from songs;",
      &SongList::sql_songlist_callback, this, &errmsg);
    if (rc != SQLITE_OK)
    {
      Logger::Error("Failed to query song database, maybe corrupted? (%s)", errmsg);
      sqlite3_free(db);
    }
    sqlite3_close(db);

    // mark hit count to check a song should be loaded
    // - also check current song is invalid or not.
    //   if invalid, it would be removed from array...
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

  // 3. Now create all invalidate song list into Task
  for (auto &check : songcheck)
  {
    if (check.hit_count == 0)
    {
      Task* t = new SongListUpdateTask(check.songpath);
      TaskPool::getInstance().EnqueueTask(t);
      is_loaded_ = false;   /* Song is not loaded yet in this state! */
    }
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
}

void SongList::Update()
{
  // check if invalidate list is done.
  // if so, mark as all loaded
  if (total_inval_size_ == load_count_ && !is_loaded_)
  {
    is_loaded_ = true;
    Save();
    EventManager::SendEvent("SongListLoaded");
  }
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

size_t SongList::size() { return songs_.size(); }
std::vector<SongListData>::iterator SongList::begin() { return songs_.begin(); }
std::vector<SongListData>::iterator SongList::end() { return songs_.end(); }
const SongListData& SongList::get(int i) const { return songs_[i]; }
SongListData SongList::get(int i) { return songs_[i]; }

void SongList::StartSongLoading(const std::string &name)
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  current_loading_file_ = name;
}

void SongList::PushChart(const SongListData &dat)
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  songs_.push_back(dat);
}

void SongList::FinishSongLoading()
{
  std::lock_guard<std::mutex> lock(loading_mutex_);
  load_count_++;
}


SongList *SONGLIST = nullptr;

}
