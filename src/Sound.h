#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include <vector>
#include "rutil.h"

namespace rhythmus
{

class GameSound;

using GameSoundAuto = std::shared_ptr<GameSound>;


/* @brief Do total sound management */
class GameMixer
{
public:
  GameMixer();
  ~GameMixer();

  void Initialize();
  void Destroy();
  void GetDevices(std::vector<std::string> &names);

  GameSoundAuto LoadSound(const std::string& path);
  GameSoundAuto LoadSound(const char* ptr, size_t len);

  static GameMixer& getInstance();

  /* @brief set buffer size. this should be called before Initialize() */
  void set_buffer_size(int bytes);

private:
  int buffer_size_;
};

/* @brief Contains real sound to be played */
class GameSound
{
public:
  GameSound();
  ~GameSound();

  void Load(const std::string& path);
  void LoadFromMemory(const char* p, size_t len);
  void LoadFromMemory(const rutil::FileData &fd);
  void Unload();
  void Play();

  bool is_loaded() const;

private:
  /* allocated channel id in mixer object
   * -1 indicates not loaded */
  int channel_id_;

  static int allocate_channel();

  friend class GameMixer;
};

}