#pragma once

#include "ResourceManager.h"
#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include "rmixer.h"

namespace rhythmus
{

class Task;

/* @brief Play sound from Mixer. */
class SoundDriver
{
public:
  SoundDriver();
  ~SoundDriver();

  void Initialize();
  void Destroy();
  void GetDevices(std::vector<std::string> &names);

  static SoundDriver& getInstance();
  static rmixer::Mixer& getMixer();

private:
  std::string device_name_;
  size_t source_count_;
  size_t buffer_size_;
  size_t channel_count_;
  unsigned audio_format_;
  bool is_running_;

  rmixer::Mixer *mixer_;
  rmixer::SoundInfo sinfo_;

  void sound_thread_body();
};


/* @brief Sound data object which contains raw data. */
class SoundData : public ResourceElement
{
public:
  SoundData();
  ~SoundData();
  bool Load(const std::string &path);
  bool Load(const char *p, size_t len, const char *name_opt);
  bool Load(const MetricGroup &m);
  void Unload();
  int get_result() const;
  rmixer::Sound *get_sound();
  bool is_empty() const;

private:
  rmixer::Sound *sound_;
  int result_;
};

/* @brief Sound object which is used for playing sound. */
class Sound
{
public:
  Sound();
  ~Sound();
  bool Load(const std::string &path);
  bool Load(const char *p, size_t len, const char *name_opt);
  bool Load(SoundData *sounddata);
  void Unload();
  void Play();
  void Stop();
  rmixer::Channel *get_channel();

  bool is_empty() const;
  bool is_loading() const;

private:
  SoundData *sounddata_;
  rmixer::Channel *channel_;
  bool is_sound_from_rm_;
};


}
