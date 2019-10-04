#include "Song.h"
#include "Timer.h"
#include "Event.h"
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
  thread_count_ = 4;
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
      "SELECT title, subtitle, artist, subartist, genre, "
      "songpath, chartpath, level, judgediff, modified_date "
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
  EventManager::SendEvent(Events::kEventSongListLoaded);

  ASSERT(thread_count_ > 0);
  for (int i = 0; i < thread_count_; ++i)
  {
    TaskAuto t = std::make_unique<SongListUpdateTask>();
    TaskPool::getInstance().EnqueueTask(t);
  }
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
      "title CHAR(128),"
      "subtitle CHAR(128),"
      "artist CHAR(128),"
      "subartist CHAR(128),"
      "genre CHAR(64),"
      "songpath CHAR(1024) NOT NULL,"
      "chartpath CHAR(512) NOT NULL,"
      "level INT,"
      "judgediff INT,"
      "modified_date INT"
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
        "'%s', '%s', '%s', '%s', '%s', '%s', '%s',"
        "%d, %d, %d"
        ");",
        s.title, s.subtitle, s.artist, s.subartist, s.genre,
        s.songpath, s.chartpath, s.level, s.judgediff, s.modified_date
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
      filepath = invalidate_list_.front();
      invalidate_list_.pop_front();
      current_loading_file_ = filepath;
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
      rparser::Chart* c = song->GetChart(i);
      auto &meta = c->GetMetaData();
      meta.SetMetaFromAttribute();
      meta.SetUtf8Encoding();
      dat.chartpath = c->GetFilename();
      // TODO: automatically extract subtitle from title
      dat.title = meta.title;
      dat.subtitle = meta.subtitle;
      dat.artist = meta.artist;
      dat.subartist = meta.subartist;
      dat.genre = meta.genre;
      dat.level = meta.level;
      dat.judgediff = meta.difficulty;

      songs_.push_back(dat);
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

int SongList::sql_songlist_callback(void *_self, int argc, char **argv, char **colnames)
{
  SongList *self = static_cast<SongList*>(_self);
  for (int i = 0; i < argc; ++i)
  {
    SongListData sdata;
    sdata.title = argv[0];
    sdata.subtitle = argv[1];
    sdata.artist = argv[2];
    sdata.subartist = argv[3];
    sdata.genre = argv[4];
    sdata.songpath = argv[5];
    sdata.chartpath = argv[6];
    sdata.level = atoi(argv[7]);
    sdata.judgediff = atoi(argv[8]);
    sdata.modified_date = atoi(argv[9]);
    self->songs_.push_back(sdata);
  }
  return 0;
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


// ----------------- class SongPlayableLoadTask

class SongPlayableChartLoadTask : public Task
{
public:
  SongPlayableChartLoadTask
  (const std::string& song_path, const std::string& chart_name)
    : song_path_(song_path), chart_name_(chart_name)
  {}

  virtual void run()
  {
    SongPlayable::getInstance().Load(song_path_, chart_name_);
  }

  virtual void abort()
  {
  }

private:
  std::string song_path_;
  std::string chart_name_;
};

class SongPlayableResourceLoadTask : public Task
{
public:
  virtual void run()
  {
    SongPlayable::getInstance().LoadResources();
  }

  virtual void abort()
  {
    SongPlayable::getInstance().CancelLoad();
  }
};

// ------------------------- class SongPlayable

SongPlayable::SongPlayable()
  : song_(nullptr), chart_(nullptr),
    note_current_idx_(0), event_current_idx_(0),
    is_loaded_(0), load_thread_count_(4), load_total_count_(0), load_count_(0),
    song_start_time_(0)
{
  // XXX: 2048 is proper size?
  keysound_.Initalize(2048);
}

void SongPlayable::Load(const std::string& path, const std::string& chartpath)
{
  // XXX: change variable to RAII type?
  is_loaded_ = 1;
  tasks_.clear();

  song_ = new rparser::Song();
  if (!song_->Open(path))
  {
    std::cerr << "SongPlayable: Failed to open song " <<
      path << std::endl;
    is_loaded_ = 0;
    return;
  }
  chart_ = song_->GetChart(chartpath);
  if (!chart_)
  {
    std::cerr << "SongPlayable: Failed to open song " <<
      path << " with chartpath " << chartpath << std::endl;
    is_loaded_ = 0;
    return;
  }
  chart_->Invalidate();

  // make note data first ...
  auto &notedata = chart_->GetNoteData();
  for (auto &note : notedata)
  {
    SongNote n;
    n.beat = note.beat;
    n.time = note.time_msec;
    // if is scorable --> TapNote / else --> BgaNote
    if (note.IsScoreable())
      n.lane = note.GetLane();
    else
      n.lane = -1;
    n.channel = note.value;
    notes_.push_back(n);
  }
  note_current_idx_ = 0;

  auto &eventdata = chart_->GetEventNoteData();
  for (auto &evnt : eventdata)
  {
    EventNote n;
    n.type = 1;
    n.time = evnt.time_msec;
    n.channel = 0; //evnt.type; // TODO: make get_value method.
    events_.push_back(n);
  }
  event_current_idx_ = 0;

  auto &metadata = chart_->GetMetaData();
  for (auto& snd : metadata.GetSoundChannel()->fn)
  {
    LoadInfo li;
    li = { 0, (int)snd.first, snd.second };
    loadinfo_list_.push_back(li);
  }
  for (auto& bga : metadata.GetBGAChannel()->bga)
  {
    LoadInfo li;
    li = { 1, (int)bga.first, bga.second.fn };
    loadinfo_list_.push_back(li);
  }
  load_total_count_ = loadinfo_list_.size();
  load_count_ = 0;
  is_loaded_ = 0;

  // preparation before calling loader thread
  keysound_.LoadFromChart(*song_, *chart_);

  // create loader task
  for (int i = 0; i < load_thread_count_; ++i)
  {
    TaskAuto t = std::make_shared<SongPlayableResourceLoadTask>();
    tasks_.push_back(t);
    TaskPool::getInstance().EnqueueTask(t);
  }
}

void SongPlayable::LoadAsync(const std::string& path, const std::string& chartpath)
{
  if (IsLoading())
    return;

  TaskAuto t = std::make_shared<SongPlayableChartLoadTask>(
    path, chartpath
    );
  TaskPool::getInstance().EnqueueTask(t);
}

void SongPlayable::LoadResources()
{
  while (!keysound_.is_loading_finished())
    keysound_.LoadRemainingSound();

  while (true)
  {
    LoadInfo li;
    {
      std::lock_guard<std::mutex> lock(loadinfo_list_mutex_);
      if (loadinfo_list_.empty())
        break;
      li = loadinfo_list_.back();
      loadinfo_list_.pop_back();
    }

    rparser::Directory* dir = song_->GetDirectory();

    if (li.type == 0 /* sound */)
    {
    }
    else if (li.type == 1 /* bga */)
    {
      const char* file;
      size_t len;
      if (!dir->GetFile(li.path, &file, len))
      {
        std::cerr << "SongPlayable: cannot read BGA :" << li.path << std::endl;
        continue;
      }
      bg_[li.channel].LoadFromData((uint8_t*)file, len);
    }
  }

  /* load done */
  is_loaded_ = 2;
}

void SongPlayable::CancelLoad()
{
  // clear all loading context
  {
    std::lock_guard<std::mutex> lock(loadinfo_list_mutex_);
    loadinfo_list_.clear();
  }

  // wait all working thread to be done
  for (auto &t : tasks_)
    t->wait();

  // last context reset
  load_total_count_ = 0;
  load_count_ = 0;
  is_loaded_ = 0;
}

void SongPlayable::Play()
{
  keysound_.RegisterToMixer(SoundDriver::getMixer());
  song_start_time_ = Timer::GetGameTimeInMillisecond();
}

void SongPlayable::Stop()
{
  song_start_time_ = 0;
  //for (int i = 0; i < 1000; ++i)
  //  keysounds_[i].Stop();
}

void SongPlayable::Update(float delta)
{
  // TODO: play bgm which is behind current timestamp
  if (!IsPlaying())
    return;

  song_current_time_ = Timer::GetGameTimeInMillisecond();
  int songtime = GetSongEclipsedTime();

  keysound_.Update(delta);

  int i = event_current_idx_;
  for (; i < events_.size(); ++i)
  {
    if (songtime >= events_[i].time)
    {
      switch (events_[i].type)
      {
      case 0: // bgm
        break;
      case 1: // bga
        // TODO: set current_bga ImageAuto.
        break;
      }
    }
    else break;
  }
  event_current_idx_ = i;
}

void SongPlayable::Clear()
{
  // should not be called while loading!
  if (IsLoading())
    return;

  keysound_.UnregisterAll();

  for (int i = 0; i < 1000; ++i)
    bg_[i].UnloadAll();

  if (song_)
  {
    song_->CloseChart();
    song_->Close();
  }

  chart_ = nullptr;
  song_ = nullptr;
  is_loaded_ = 0;
}

bool SongPlayable::IsLoading() const
{
  return is_loaded_ == 1;
}

bool SongPlayable::IsLoaded() const
{
  return is_loaded_ >= 2;
}

bool SongPlayable::IsPlaying() const
{
  return song_start_time_ > 0;
}

bool SongPlayable::IsPlayFinished() const
{
  // current check with : last object time > song eclipsed time
  return (IsPlaying() && GetSongEclipsedTime() > notes_.back().time);
}

double SongPlayable::GetProgress() const
{
  if (load_total_count_ == 0)
    return 0;
  else return (float)load_count_ / load_total_count_;
}

double SongPlayable::GetSongStartTime() const
{
  return song_start_time_;
}

int SongPlayable::GetSongEclipsedTime() const
{
  if (song_start_time_ == 0) return 0;
  return static_cast<int>(song_current_time_ - song_start_time_);
}

void SongPlayable::Input(int keycode, uint32_t gametime)
{
  // TODO
}

SongPlayable& SongPlayable::getInstance()
{
  static SongPlayable s;
  return s;
}

}