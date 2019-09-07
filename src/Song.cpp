#include "Song.h"
#include "Timer.h"
#include "Event.h"
#include <map>
#include <thread>
#include <mutex>

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
  rutil::DirFileList dir;

  // 1. attempt to read file/folder list in directory
  if (!rutil::GetDirectoryFiles(song_dir_, dir, 0))
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
    invalidate_list_.push_back(d.first);
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
  total_inval_size_ = 0;
  songs_.clear();
  invalidate_list_.clear();
  active_thr_count_ = 0;
}

double SongList::get_progress() const
{
  if (total_inval_size_ == 0)
    return 1.0;
  else
    return invalidate_list_.size() / (double)total_inval_size_;
}

bool SongList::is_loading() const
{
  return !invalidate_list_.empty() && active_thr_count_ > 0;
}

std::string SongList::get_loading_filename() const
{
  if (invalidate_list_.empty())
    return std::string();
  else
    return invalidate_list_.back();
}

SongList& SongList::getInstance()
{
  static SongList s;
  return s;
}

size_t SongList::size() { return songs_.size(); }
std::vector<SongAuto>::iterator SongList::begin() { return songs_.begin(); }
std::vector<SongAuto>::iterator SongList::end() { return songs_.end(); }
const SongAuto& SongList::get(int i) const { return songs_[i]; }
SongAuto SongList::get(int i) { return songs_[i]; }

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
    filepath = l.get_loading_filename();
    invalidate_list_.pop_back();
    lock.unlock();

    // attempt song loading.
    SongAuto song = std::make_shared<Song>();
    if (!song->Load(filepath))
      continue;
    songs_.push_back(song);
  }

  // - end loading -
  active_thr_count_--;
  if (active_thr_count_ == 0)
  {
    EventManager::SendEvent(Events::kEventSongListLoadFinished);
  }
  std::cout << "SongList: Loading thread finished." << std::endl;
}

// --------------------------------- class Song

bool Song::Load(const std::string& path)
{
  return song_.Open(path);
}

void SongPlayable::Play()
{
}

void SongPlayable::Stop()
{
}

bool SongPlayable::IsPlaying()
{
  return false;
}

double SongPlayable::GetSongStartTime()
{
  return 0;
}

double SongPlayable::GetSongEclipsedTime()
{
  return 0;
}

}