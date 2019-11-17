#pragma once

#include "Timer.h"
#include "Setting.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <map>

namespace rhythmus
{

/* @brief Current game boot mode. */
enum GameBootMode
{
  kBootNormal, /* normal boot mode (homeplay) */
  kBootArcade, /* arcade boot mode */
  kBootLR2, /* LR2 type boot mode */
  kBootPlay, /* Only for play */
  kBootRefresh, /* Only for library refresh */
  kBootTest, /* hidden boot mode - only for test purpose */
};

/**
 * @brief
 * Contains running status of game, including settings.
 */
class Game
{
public:
  static Game& getInstance();
  void Default();

  /* Update game status. */
  void Update();

  /* Load execute argument */
  void LoadArgument(const std::string& argv);

  template<typename T>
  T GetAttribute(const std::string& key) const;
  void SetAttribute(const std::string& key, const std::string& value);

  void set_setting_path(const std::string& path);
  uint16_t get_window_width() const;
  uint16_t get_window_height() const;
  std::string get_window_title() const;
  std::string get_log_path() const;
  bool get_do_logging() const;
  float GetAspect() const;
  GameBootMode get_boot_mode() const;
  void push_song(const std::string& songpath);
  bool pop_song(std::string& songpath);

  void set_do_logging(bool v);

private:
  Game();
  ~Game();

  static Game game_;

  std::string setting_path_;

  uint16_t width_, height_;
  std::string log_path_;
  bool do_logging_;

  // misc attributes
  std::map<std::string, std::string> attributes_;

  // current game boot mode.
  GameBootMode game_boot_mode_;

  // song path list to play
  std::list<std::string> song_queue_;

  void InitializeGameOption();
  void ApplyGameOption();
};

}