#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include "rmixer.h"
#include "rutil.h"

namespace rhythmus
{

using Sound = rmixer::Sound;

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