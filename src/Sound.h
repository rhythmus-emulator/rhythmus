#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include "rmixer.h"

namespace rhythmus
{

using Sound = rmixer::Sound;

/* @brief Sound used with scene resource */
class GameSound : public rmixer::Sound
{
public:
  void set_name(const std::string &name);
  void Load();
  void LoadWithName(const std::string &name);

  /* overriden function */
  void Load(const std::string &path);

private:
  std::string name_;
};

/* @brief Do total sound management */
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
  rmixer::Mixer mixer_;
  size_t buffer_size_;

  void sound_thread_body();
};

}
