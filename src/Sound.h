#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>

namespace rhythmus
{

class Sound;

using SoundAuto = std::shared_ptr<Sound>;


/* @brief Do total sound management */
class Mixer
{
public:
  SoundAuto LoadSound(const std::string& path);
  SoundAuto LoadSound(const char* ptr, size_t len);

  /* @warn Sound is automatically unloaded when destructed. */
  void UnloadSound(Sound* s);

  static Mixer& getInstance();

private:
  std::vector<Sound*> sounds_;
};

/* @brief Contains real sound to be played */
class Sound
{
public:
  Sound();
  ~Sound();

private:
  bool is_loaded_;

  friend class Mixer;
};

}