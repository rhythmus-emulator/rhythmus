#pragma once

#include "Image.h"
#include "Sound.h"
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <list>
#include <vector>
#include <thread>

// rmixer
#include "SoundPool.h"

namespace rparser { class Song; class Chart; }

namespace rhythmus
{

struct SongListData
{
  std::string title;
  std::string subtitle;
  std::string artist;
  std::string subartist;
  std::string genre;
  std::string songpath;
  std::string chartpath;
  int level;
  int judgediff;
  int modified_date;
};

/* @brief A singleton class which contains currently loaded Song DB */
class SongList
{
public:
  SongList();

  void Load();
  void Save();
  void Clear();

  /* @brief invalidate single song entry. used for multi-threading. */
  void InvalidateSingleSongEntry();

  double get_progress() const;
  bool is_loaded() const;
  std::string get_loading_filename() const;

  size_t size();
  std::vector<SongListData>::iterator begin();
  std::vector<SongListData>::iterator end();
  const SongListData& get(int i) const;
  SongListData get(int i);

  static SongList& getInstance();

private:
  // loaded songs
  std::vector<SongListData> songs_;

  // songs to invalidate
  std::list<std::string> invalidate_list_;
  std::string current_loading_file_;
  int total_inval_size_;
  std::atomic<int> load_count_;
  bool is_loaded_;

  std::string song_dir_;

  int thread_count_;

  static int sql_dummy_callback(void*, int argc, char **argv, char **colnames);
  static int sql_songlist_callback(void*, int argc, char **argv, char **colnames);
};

/* @brief A singleton class. Song data with playing context. */
class SongPlayable
{
public:
  SongPlayable();
  void Load(const std::string& path, const std::string& chartpath);
  void CancelLoad();
  void Play();
  void Stop();
  void Update(float delta);
  void Clear();

  bool IsLoading() const;
  bool IsLoaded() const;
  bool IsPlaying() const;
  bool IsPlayFinished() const;
  double GetProgress() const;
  double GetSongStartTime() const;
  int GetSongEclipsedTime() const;

  /* @brief make judgement, sound, and score change with input */
  void Input(int keycode, uint32_t gametime);

  static SongPlayable& getInstance();

private:
  void LoadResourceThreadBody();

private:
  rparser::Song* song_;
  rparser::Chart* chart_;
  int is_loaded_;

  /* tappable notes */
  struct SongNote {
    int lane;
    int channel;
    int time;
    double beat;
  };
  std::vector<SongNote> notes_;
  int note_current_idx_;

  /* events like bg / trigger / bgm ... */
  struct EventNote {
    int type; // 0: bgm, 1: bga
    int channel;
    int time;
  };
  std::vector<EventNote> events_;
  int event_current_idx_;

  int load_thread_count_;
  std::atomic<int> active_thread_count_;
  int load_total_count_;
  std::atomic<int> load_count_;
  struct LoadInfo
  {
    int type; // 0: sound, 1: image
    int channel;
    std::string path;
  };
  std::list<LoadInfo> loadinfo_list_;
  std::list<std::thread> load_thread_;
  std::mutex loadinfo_list_mutex_;

  uint32_t song_start_time_;
  uint32_t song_current_time_;
  
  rmixer::KeySoundPoolWithTime keysound_;
  Image bg_[1000];

  // TODO: input settings.
};

}