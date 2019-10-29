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

  size_t thread_count_ = TaskPool::getInstance().GetPoolSize();
  for (size_t i = 0; i < thread_count_; ++i)
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

  song_selected_.level = 0;
  song_selected_.judgediff = 0;
  song_selected_.modified_date = 0;
  song_selected_.title = "(no title)";
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
SongListData* SongList::get_current_song_info() { return &song_selected_; }
void SongList::select(int i) { if (i >= 0 && i < songs_.size()) song_selected_ = songs_[i]; }


// ----------------- class SongResourceLoadTask

class SongResourceLoadTask : public Task
{
public:
  SongResourceLoadTask
  (SongResource *res, const std::string& song_path)
    : res_(res), song_path_(song_path)
  {}

  virtual void run()
  {
    res_->Load(song_path_);
  }

  virtual void abort()
  {
  }

private:
  SongResource *res_;
  std::string song_path_;
};

class SongResourceLoadResourceTask : public Task
{
public:
  SongResourceLoadResourceTask(SongResource *res)
    : res_(res) {}

  virtual void run()
  {
    res_->LoadResources();
  }

  virtual void abort()
  {
    res_->CancelLoad();
  }

private:
  SongResource *res_;
};

// ------------------------- class SongResource

SongResource::SongResource()
  : song_(nullptr), is_loaded_(0), load_bga_(true)
{
}

void SongResource::Load(const std::string& path)
{
  // already loaded?
  if (is_loaded_ > 0) return;

  bool load_result = true;
  is_loaded_ = 1;

  song_ = new rparser::Song();
  load_result = song_->Open(path);

  if (!load_result)
  {
    delete song_;
    is_loaded_ = 0;
    return;
  }

  // create resource list to load
  FileToLoad load;
  for (auto *file : *song_->GetDirectory())
  {
    load.filename = file->filename;
    std::string ext = GetExtension(file->filename);
    if (ext == "wav" || ext == "ogg" || ext == "flac" || ext == "mp3")
    {
      load.type = 1;
    }
    else if (ext == "bmp" || ext == "gif" || ext == "jpg" || ext == "jpeg" ||
      ext == "tiff" || ext == "tga" || ext == "png" || ext == "avi" || ext == "mpg" ||
      ext == "mpeg" || ext == "mp4")
    {
      if (!load_bga_) continue;
      load.type = 0;
    }
    else continue;
    files_to_load_.push_back(load);
  }

  // create resource loader task
  loadfile_count_total_ = files_to_load_.size();
  loaded_file_count_ = 0;
  size_t load_thread_count = TaskPool::getInstance().GetPoolSize();
  for (int i = 0; i < load_thread_count; ++i)
  {
    TaskAuto t = std::make_shared<SongResourceLoadResourceTask>(this);
    tasks_.push_back(t);
    TaskPool::getInstance().EnqueueTask(t);
  }
}

void SongResource::LoadAsync(const std::string& path)
{
  TaskAuto t = std::make_shared<SongResourceLoadTask>(this, path);
  tasks_.push_back(t);
  TaskPool::getInstance().EnqueueTask(t);
}


/* internally called from Load() and LoadAsync() */
void SongResource::LoadResources()
{
  while (1)
  {
    FileToLoad file_to_load;
    std::string fn;
    {
      std::lock_guard<std::mutex> lock(loading_mutex_);
      if (files_to_load_.empty())
        break;
      file_to_load = files_to_load_.front();
      files_to_load_.pop_front();
      loaded_file_count_++;
    }
    fn = file_to_load.filename;

    const char* file;
    size_t len;
    if (song_->GetDirectory()->GetFile(fn, &file, len))
    {
      if (file_to_load.type == 0)
      {
        // image
        Image *img = new Image();
        img->LoadFromData((uint8_t*)file, len);
        bgs_.push_back({ fn, img });
      }
      else if (file_to_load.type == 1)
      {
        // sound
        Sound *snd = new Sound();
        snd->Resample(SoundDriver::getInstance().getMixer().GetSoundInfo());
        snd->Load(file, len);
        sounds_.push_back({ fn, snd });
      }
    }
  }

  is_loaded_ = 2;
}

/* This function should be called after all song resources are loaded */
void SongResource::UploadBitmaps()
{
  for (auto &bg : bgs_) if (bg.second->is_loaded())
    bg.second->CommitImage();
}

/* For updating image */
void SongResource::Update(float delta)
{
  for (auto &bg : bgs_) if (bg.second->is_loaded())
    bg.second->Update(delta);
}

void SongResource::CancelLoad()
{
  {
    std::lock_guard<std::mutex> lock(loading_mutex_);
    files_to_load_.clear();
  }
  for (auto& t : tasks_)
    t->wait();
  tasks_.clear();
}

void SongResource::Clear()
{
  CancelLoad();
  for (auto &bg : bgs_)
    delete bg.second;
  for (auto &sound : sounds_)
    delete sound.second;
  bgs_.clear();
  sounds_.clear();
  song_->Close();
  delete song_;
  song_ = nullptr;
  is_loaded_ = 0;
}

rparser::Song* SongResource::get_song()
{
  return song_;
}

void SongResource::set_load_bga(bool use_bga)
{
  load_bga_ = use_bga;
}

int SongResource::is_loaded() const
{
  return is_loaded_;
}

double SongResource::get_progress() const
{
  if (loadfile_count_total_ == 0) return .0;
  return (double)loaded_file_count_ / loadfile_count_total_;
}

Sound* SongResource::GetSound(const std::string& filename)
{
  for (auto &snd : sounds_)
    if (snd.first == filename) return snd.second;
  return nullptr;
}

Image* SongResource::GetImage(const std::string& filename)
{
  for (auto &bg : bgs_)
    if (bg.first == filename) return bg.second;
  return nullptr;
}

SongResource& SongResource::getInstance()
{
  static SongResource r;
  return r;
}

}