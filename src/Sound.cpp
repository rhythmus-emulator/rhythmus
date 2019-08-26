#include "Sound.h"
#include <algorithm>

namespace rhythmus
{

// -------------------------------- class Mixer

SoundAuto Mixer::LoadSound(const std::string& path)
{
  SoundAuto s = std::make_unique<Sound>();
  s->is_loaded_ = true;
  return s;
}

SoundAuto Mixer::LoadSound(const char* ptr, size_t len)
{
  SoundAuto s = std::make_unique<Sound>();
  s->is_loaded_ = true;
  return s;
}

/* @warn Sound is automatically unloaded when destructed. */
void Mixer::UnloadSound(Sound* s)
{
  if (!s->is_loaded_)
    return;

  s->is_loaded_ = false;
  sounds_.erase(std::find(sounds_.begin(), sounds_.end(), s));
}

Mixer& Mixer::getInstance()
{
  static Mixer mixer;
  return mixer;
}


// -------------------------------- class Sound


Sound::Sound()
  : is_loaded_(false)
{
}

Sound::~Sound()
{
  Mixer::getInstance().UnloadSound(this);
}


}