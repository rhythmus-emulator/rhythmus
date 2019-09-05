#pragma once

#include "rhythmus.h"
#include "Sound.h"
#include <memory>
#include <vector>

namespace rhythmus
{

class Song;
using SongAuto = std::shared_ptr<Song>;

/* @brief A singleton class which contains currently loaded Song DB */
class SongList
{
public:
  void Load();
  void Save();
private:
  std::vector<SongAuto> songs_;
};

/* @brief Song data */
class Song
{
public:
  bool Load(const std::string& path);

private:
  rparser::Song song_;

  friend class SongList;
};

/* @brief A singleton class. currently playable song */
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