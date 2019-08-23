#pragma once

#include "Timer.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace rhythmus
{

/* @brief Current game boot mode. */
enum GameBootMode
{
  kBootNormal,
  kBootTest, /* hidden boot mode - only for test purpose */
};

/* @brief current game mode. */
enum GameMode
{
  kGameModeNone,
  kGameModeMain,
  kGameModeLogin,
  kGameModeSelectMode,
  kGameModeSelect,
  kGameModeDecide,
  kGameModePlay,
  kGameModeResult,
  kGameModeCourseResult,
  kGameModeSetting,
  kGameClose /* End of a game, Unexitable game state. */,
  kGameModeKeySetting /* Special mode, only for LR2 */,
  kGameModeTest /* Special mode, only for testing */,
};

/* @brief game theme related options */
struct GameThemeOption
{
  // general scene master path, which includes parameter for total scene
  // other parameter filled automatically if null.
  std::string scene_path;

  // select scene path
  std::string select_scene_path;

  // decide scene path
  std::string decide_scene_path;

  // play scene path
  std::string play_scene_path;

  // result scene path
  std::string result_path;

  // course result scene path
  std::string courseresult_scene_path;
};

/**
 * @brief
 * Contains running status of game, including settings.
 */
class Game
{
public:
  static Game& getInstance();
  bool Load();
  bool Save();
  void Default();
  void LoadOrDefault();

  /* Update game status. */
  void Update();

  /* Set next game mode */
  void SetNextGameMode(GameMode next_game_mode);

  /* Trigger scene changing. */
  void ChangeGameMode();

  void set_setting_path(const std::string& path);
  uint16_t get_window_width() const;
  uint16_t get_window_height() const;
  std::string get_window_title() const;
  std::string get_log_path() const;
  bool get_do_logging() const;
  float GetAspect() const;
  GameBootMode get_boot_mode() const;
  GameMode get_game_mode() const;

  void set_do_logging(bool v);

private:
  Game();
  ~Game();

  static Game game_;

  std::string setting_path_;
  uint16_t width_, height_;
  std::string log_path_;
  bool do_logging_;

  // game theme related variables.
  GameThemeOption theme_option_;

  // current game boot mode.
  GameBootMode game_boot_mode_;

  // current game mode.
  GameMode game_mode_;

  // reserved next game mode.
  GameMode next_game_mode_;

  // check is game mode (scene) need to be changed.
  bool do_game_mode_change_;
};

}