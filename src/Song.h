#pragma once

#include "rhythmus.h"
#include "Sound.h"
#include <string>
#include <memory>
#include <vector>

namespace rhythmus
{

class Song;
using SongAuto = std::shared_ptr<rparser::Song>;

/* @brief A singleton class which contains currently loaded Song DB */
class SongList
{
public:
  SongList();

  void Load();
  void Save();
  void Clear();

  double get_progress() const;
  bool is_loading() const;
  std::string get_loading_filename() const;

  size_t size();
  std::vector<SongAuto>::iterator begin();
  std::vector<SongAuto>::iterator end();
  const SongAuto& get(int i) const;
  SongAuto get(int i);

  static SongList& getInstance();

private:
  // loaded songs
  std::vector<SongAuto> songs_;

  // songs to invalidate
  std::vector<std::string> invalidate_list_;
  int total_inval_size_;
  int active_thr_count_;

  std::string song_dir_;

  int thread_count_;

  /* @brief read directory and create song path list */
  void PrepareSongpathlistFromDirectory();

  void song_loader_thr_body();
};

/* @brief Song data optimized for game context. */
class Song
{
public:
  bool Load(const std::string& path);

private:
  rparser::Song song_;

  friend class SongList;
};

/* @brief A singleton class. song data with playing context. */
class SongPlayable : public Song
{
public:
  void Play();
  void Stop();

  bool IsPlaying();
  double GetSongStartTime();
  double GetSongEclipsedTime();

private:
  std::vector<SoundAuto> keysounds_;
};

}