#include "SongPlayer.h"
#include "Song.h" /* SongPlayinfo class */
#include "Player.h"
#include "Util.h"
#include "Logger.h"

namespace rhythmus
{

// ----------------- class SongResourceLoadTask

class SongResourceLoadTask : public Task
{
public:
  SongResourceLoadTask(SongPlayer *res) : res_(res) {}
  virtual void run() { res_->Load(); }
  virtual void abort() {}
private:
  SongPlayer *res_;
};

class SongResourceLoadResourceTask : public Task
{
public:
  SongResourceLoadResourceTask(SongPlayer *res) : res_(res) {}
  virtual void run() { res_->LoadResources(); }
  virtual void abort() { res_->Stop(); }
private:
  SongPlayer *res_;
};

// ------------------------- class SongResource

SongPlayer::SongPlayer()
  : song_(nullptr), is_loaded_(0), load_async_(true), load_bga_(true)
{
}

bool SongPlayer::Load()
{
  bool r;

  /* if started loading, then do nothing. */
  if (is_loaded_ != 0)
    return true;

  /* load song (sync) */
  auto *pctx = GetSongPlayinfo();
  if (!pctx)
  {
    Logger::Error("Attempt to play song but no song queued for playing.");
    return false;
  }
  std::string path = pctx->songpath;
  song_ = new rparser::Song();
  r = song_->Open(path);
  if (!r)
  {
    Logger::Error("Song loading failed: %s", path.c_str());
    delete song_;
    return false;
  }
  is_loaded_ = 1;

  /* Initialize each player for chart */
  FOR_EACH_PLAYER(p, i)
  {
    rparser::Chart *c = song_->GetChart(pctx->chartpaths[0]);
    if (!c)
    {
      Logger::Error("Attempt to play chart %s but not existing.",
        pctx->chartpaths[0].c_str());
      continue;
    }
    c->Invalidate();
    p->SetChart(*c);
  }
  END_EACH_PLAYER()

  /* prepare resource list to load */
  PrepareResourceListFromSong();

  /* load image & audio */
  if (load_async_)
  {
    LoadResourcesAsync();
  }
  else
  {
    LoadResources();
    is_loaded_ = 2;
  }

  return true;
}

void SongPlayer::Play()
{
  /* if not loaded, then do loading first. */
  if (is_loaded_ == 0)
    Load();
  else if (is_loaded_ != 2)
    return;

  /* Push charts into players */
  FOR_EACH_PLAYER(p, i)
  {
    p->StartPlay();
  }
  END_EACH_PLAYER()
}

void SongPlayer::Stop()
{
  if (is_loaded_ == 0)
    return;

  /* stop loading thread */
  CancelLoad();

  /* Release player chart. */
  FOR_EACH_PLAYER(p, i)
  {
    p->FinishPlay();
  }
  END_EACH_PLAYER()

  /* lock and clear all resources. */
  std::lock_guard<std::mutex> lock(loading_mutex_);
  files_to_load_.clear();

  for (auto &bg : bg_animations_)
    delete bg.second;
  for (auto &sound : sounds_)
    delete sound.second;
  bg_animations_.clear();
  sounds_.clear();
  if (song_)
  {
    song_->Close();
    delete song_;
    song_ = nullptr;
  }
  is_loaded_ = 0;

  /* Pop current song info. */
  PopSongFromPlaylist();
}

void SongPlayer::Update(float delta)
{
  /* upload bitmap if not uploaded
   * XXX: heavy? */
  {
    std::lock_guard<std::mutex> lock(res_mutex_);
    UploadBitmaps();
  }

  /* update movie */
  for (auto &bg : bg_animations_) if (bg.second->is_loaded())
    bg.second->Update(delta);

  /* Update all players */
  FOR_EACH_PLAYER(p, i)
  {
    if (p->GetPlayContext())
      p->GetPlayContext()->Update(delta);
  }
  END_EACH_PLAYER()
}

void SongPlayer::SetCoursetoPlay(const std::string &coursepath)
{
  // TODO
  ASSERT(0);
}

void SongPlayer::SetSongtoPlay(const std::string &songpath, const std::string &chartpath)
{
  ClearPlaylist();
  // TODO: if songpath is course, then call SetCoursetoPlay()
  AddSongtoPlaylist(songpath, chartpath);
}

void SongPlayer::AddSongtoPlaylist(const std::string &songpath, const std::string &chartpath)
{
  // if songpath contains chartpath, then automatically fill chartpath.
  SongPlayinfo playinfo;
  if (chartpath.empty() && IsChartPath(songpath))
  {
    std::string songpath_new, chartpath_new;
    auto p = songpath.find_last_of('/');
    if (p != std::string::npos)
    {
      songpath_new = songpath.substr(0, p);
      chartpath_new = songpath.substr(p + 1);
    }
    playinfo.songpath = songpath_new;
    playinfo.chartpaths[0] = chartpath_new;
  }
  else
  {
    playinfo.songpath = songpath;
    playinfo.chartpaths[0] = chartpath;
  }
  playlist_.push_back(playinfo);
}

const SongPlayinfo *SongPlayer::GetSongPlayinfo() const
{
  if (!playlist_.empty())
    return &playlist_.front();
  return nullptr;
}

void SongPlayer::PopSongFromPlaylist()
{
  playlist_.pop_front();
}

void SongPlayer::ClearPlaylist()
{
  playlist_.clear();
}

#if 0
void SongPlayer::LoadSongAsync(const std::string& path)
{
  TaskAuto t = std::make_shared<SongResourceLoadTask>(this, path);
  tasks_.push_back(t);
  TaskPool::getInstance().EnqueueTask(t);
}
#endif

void SongPlayer::PrepareResourceListFromSong()
{
  auto *dir = song_->GetDirectory();
  if (!dir)
  {
    /* some types of song has no resource directory. */
    return;
  }
  FileToLoad load;
  for (auto *file : *dir)
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
}

/* internally called from Load() and LoadAsync() */
void SongPlayer::LoadResources()
{
  if (!song_) return;

  /* If no resources are designated to be loaded,
   * Automatically create load list */
  if (is_loaded_ == 0)
  {
    std::lock_guard<std::mutex> lock(loading_mutex_);
    if (files_to_load_.empty())
    {
      PrepareResourceListFromSong();
    }
    is_loaded_ = 1;
  }

  /* Main resource loading loop */
  while (is_loaded_ == 1)
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
        {
          std::lock_guard<std::mutex> lock(res_mutex_);
          bg_animations_.push_back({ { fn }, img });
        }
      }
      else if (file_to_load.type == 1)
      {
        // sound
        Sound *snd = new Sound();
        snd->get_buffer()->Resample(
          SoundDriver::getInstance().getMixer().GetSoundInfo()
        );
        snd->Load(file, len);
        {
          std::lock_guard<std::mutex> lock(res_mutex_);
          sounds_.push_back({ { fn, -1 }, snd });
        }
      }
    }
  }

  is_loaded_ = 2;
}

void SongPlayer::LoadResourcesAsync()
{
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

/* This function should be called after all song resources are loaded */
void SongPlayer::UploadBitmaps()
{
  for (auto &bg : bg_animations_) if (bg.second->is_loaded())
    bg.second->CommitImage();
}

void SongPlayer::CancelLoad()
{
  {
    std::lock_guard<std::mutex> lock(loading_mutex_);
    files_to_load_.clear();
  }
  for (auto& t : tasks_)
    t->wait();
  tasks_.clear();
}

bool SongPlayer::IsChartPath(const std::string &path)
{
  // TODO: use rparser library later.
  std::string path_lower = Lower(path);
  return (endsWith(path_lower, ".bms")
    || endsWith(path_lower, ".bme")
    || endsWith(path_lower, ".bml")
    || endsWith(path_lower, ".osu")
    || endsWith(path_lower, ".sm"));
}

rparser::Song* SongPlayer::get_song()
{
  return song_;
}

rparser::Chart* SongPlayer::get_chart(const std::string& chartname)
{
  if (!song_)
    return nullptr;
  return song_->GetChart(chartname);
}

void SongPlayer::set_load_bga(bool use_bga)
{
  load_bga_ = use_bga;
}

int SongPlayer::is_loaded() const
{
  return is_loaded_;
}

double SongPlayer::get_progress() const
{
  if (loadfile_count_total_ == 0) return .0;
  return (double)loaded_file_count_ / loadfile_count_total_;
}

Sound* SongPlayer::GetSound(const std::string& filename, int channel)
{
  Sound *sound_found = nullptr;
  Sound *new_sound = nullptr;

  for (auto &snd : sounds_)
  {
    if (CompareFilename(snd.first.name, filename))
    {
      sound_found = snd.second;

      // -1 channel value: nobody assigned, so take it.
      if (snd.first.channel == -1)
      {
        snd.first.channel = channel;
        snd.second->RegisterToMixer(&SoundDriver::getInstance().getMixer());
      }

      // if sound found with same channel, return it directly.
      if (snd.first.channel == channel)
      {
        return snd.second;
      }
    }
  }

  // not found; return nullptr
  if (!sound_found) return nullptr;

  // sound found, but different channel.
  // therefore, create new sound by shallow_clone.
  new_sound = sound_found->shallow_clone();
  new_sound->RegisterToMixer(&SoundDriver::getInstance().getMixer());
  sounds_.push_back({ { filename, channel }, new_sound });
  return new_sound;
}

Image* SongPlayer::GetImage(const std::string& filename)
{
  for (auto &bg : bg_animations_)
    if (bg.first.name == filename) return bg.second;
  return nullptr;
}

SongPlayer& SongPlayer::getInstance()
{
  static SongPlayer r;
  return r;
}

}