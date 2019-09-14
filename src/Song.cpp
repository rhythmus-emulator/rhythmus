#include "Song.h"
#include "Timer.h"
#include "Event.h"
#include "Util.h"
#include <map>

namespace rhythmus
{

// ----------------------------- class SongList

SongList::SongList()
{
  song_dir_ = "../songs";
  thread_count_ = 4;
  Clear();
}

void SongList::Load()
{
  Clear();

  /* (path, timestamp) key */
  std::map<std::string, int> mod_dir;
  std::map<std::string, int> mod_db;
  std::vector<DirItem> dir;

  // 1. attempt to read file/folder list in directory
  if (!GetDirectoryItems(song_dir_, dir))
  {
    std::cerr <<
      "SongList: Failed to generate songlist. make sure song library path exists." <<
      std::endl;
    return;
  }

  // 2. attempt to load DB (TODO)


  // 3. comparsion modified date with DB to invalidate any song necessary. (TODO)
  for (auto &d : dir)
  {
    invalidate_list_.push_back(d.filename);
  }
  total_inval_size_ = invalidate_list_.size();
  EventManager::SendEvent(Events::kEventSongListLoaded);

  // 4. Now load all songs with multi-threading
  ASSERT(thread_count_ > 0);
  active_thr_count_ = thread_count_;
  for (int i = 0; i < thread_count_; ++i)
  {
    std::thread th(&SongList::song_loader_thr_body, this);
    th.detach();
  }
}

void SongList::Save()
{
  // TODO
}

void SongList::Clear()
{
  active_thr_count_ = 0;
  total_inval_size_ = 0;
  load_count_ = 0;
  songs_.clear();
  invalidate_list_.clear();
}

double SongList::get_progress() const
{
  if (total_inval_size_ == 0)
    return 1.0;
  else
    return (double)load_count_ / total_inval_size_;
}

bool SongList::is_loading() const
{
  return active_thr_count_ > 0;
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

void SongList::PrepareSongpathlistFromDirectory()
{

}

void SongList::song_loader_thr_body()
{
  static std::mutex lock;
  std::string filepath;
  auto& l = SongList::getInstance();

  while (true)
  {
    // get song name safely.
    // if empty, then exit loop.
    lock.lock();
    if (l.invalidate_list_.empty())
    {
      lock.unlock();
      break;
    }
    filepath = song_dir_ + "/" + invalidate_list_.front();
    invalidate_list_.pop_front();
    current_loading_file_ = filepath;
    lock.unlock();

    // attempt song loading.
    SongAuto song = std::make_shared<rparser::Song>();
    if (!song->Open(filepath))
      continue;
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

    // afterwork loading.
    load_count_++;
  }

  // - end loading -
  active_thr_count_--;
  if (active_thr_count_ == 0)
  {
    current_loading_file_.clear();
    EventManager::SendEvent(Events::kEventSongListLoadFinished);
  }
  std::cout << "SongList: Loading thread finished." << std::endl;
}


// ------------------------- class SongPlayable

SongPlayable::SongPlayable()
  : note_current_idx_(0), event_current_idx_(0),
    load_thread_count_(4), active_thread_count_(0),
    load_total_count_(0), load_count_(0), song_start_time_(0)
{
}

void SongPlayable::Load(const std::string& path, const std::string& chartpath)
{
  if (!song_.Open(path))
  {
    std::cerr << "SongPlayable: Failed to open song " << path << std::endl;
    return;
  }
  chart_ = song_.GetChart(chartpath);
  if (!chart_)
  {
    std::cerr << "SongPlayable: Failed to open song " << path << " with chartpath " << chartpath << std::endl;
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
    n.lane = note.GetLane();
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

  // create loader thread
  active_thread_count_ = load_thread_count_;
  for (int i = 0; i < load_thread_count_; ++i)
  {
    load_thread_.emplace_back(
      std::thread(&SongPlayable::LoadResourceThreadBody, this)
    );
    load_thread_.back().detach();
  }

  song_.CloseChart();
}

void SongPlayable::LoadResourceThreadBody()
{
  while (active_thread_count_ > 0)
  {
    LoadInfo li;
    loadinfo_list_mutex_.lock();
    if (loadinfo_list_.empty())
    {
      loadinfo_list_mutex_.unlock();
      break;
    }
    li = loadinfo_list_.back();
    loadinfo_list_.pop_back();
    loadinfo_list_mutex_.unlock();

    rparser::Directory* dir = song_.GetDirectory();

    if (li.type == 0 /* sound */)
    {
      rparser::FileData *fd = dir->Get(li.path);
      keysounds_[li.channel].LoadFromMemory(*fd);
#if 0
      /// future code
      rparser::FileSystem::SetBaseDirectory(song_.get_path());
      for (auto& snd : metadata.GetSoundChannel()->fn)
      {
        FileData fd = rparser::FileSystem::GetFileData();
        keysounds_[snd.first].LoadFromMemory(fd);
      }
      rparser::FileSystem::ResetBaseDirectory();
#endif
    }
    else if (li.type == 1 /* bga */)
    {
      rparser::FileData *fd = dir->Get(li.path);
      bg_[li.channel].LoadFromData(fd->p, fd->len);
    }
  }

  active_thread_count_--;
  /* should be called only once! */
  if (active_thread_count_ == 0)
  {
    FinishLoadResource();
  }
}

void SongPlayable::FinishLoadResource()
{
  // wait all working thread to be done
  active_thread_count_ = 0;
  for (auto& thr : load_thread_)
    thr.join();

  // reset all loading context
  load_total_count_ = 0;
  load_count_ = 0;

  // trigger event
  EventManager::SendEvent(Events::kEventSongLoadFinished);
}

void SongPlayable::CancelLoad()
{
  // wait all working thread to be done
  active_thread_count_ = 0;
  for (auto& thr : load_thread_)
    thr.join();

  // reset all loading context
  load_total_count_ = 0;
  load_count_ = 0;

  // clear song data
  Clear();
}

void SongPlayable::Play()
{
  song_start_time_ = Timer::GetGameTimeInMillisecond();
  int songtime = GetSongEclipsedTime();

  int i = note_current_idx_;
  for (; i < notes_.size(); ++i)
  {
    // TODO: currently autoplay ...
    if (notes_[i].time > songtime)
      keysounds_[notes_[i].channel].Play();
  }
  note_current_idx_ = i;

  i = event_current_idx_;
  for (; i < events_.size(); ++i)
  {
    if (events_[i].time > songtime)
    {
      switch (events_[i].type)
      {
      case 0: // bgm
        keysounds_[events_[i].channel].Play();
        break;
      case 1: // bga
        // TODO: set current_bga ImageAuto.
        break;
      }
    }
  }
}

void SongPlayable::Stop()
{
  song_start_time_ = 0;
  //for (int i = 0; i < 1000; ++i)
  //  keysounds_[i].Stop();
}

void SongPlayable::Update(float)
{
  song_current_time_ = Timer::GetGameTimeInMillisecond();

  // TODO: play bgm which is behind current timestamp
}

void SongPlayable::Clear()
{
  // should not be called while loading!
  if (active_thread_count_ != 0)
    return;

  for (int i = 0; i < 1000; ++i)
    keysounds_[i].Unload();

  for (int i = 0; i < 1000; ++i)
    bg_[i].UnloadAll();
}

bool SongPlayable::IsPlaying()
{
  return song_start_time_ > 0;
}

bool SongPlayable::IsFinished()
{
  return false;
}

double SongPlayable::GetProgress()
{
  if (load_total_count_ == 0)
    return 0;
  else return (float)load_count_ / load_total_count_;
}

double SongPlayable::GetSongStartTime()
{
  return song_start_time_;
}

int SongPlayable::GetSongEclipsedTime()
{
  if (song_start_time_ == 0) return 0;
  return static_cast<int>(song_current_time_ - song_start_time_);
}

void SongPlayable::Input(int keycode, uint32_t gametime)
{
  // TODO
}

}