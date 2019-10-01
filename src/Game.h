#pragma once

#include "Timer.h"
#include "Setting.h"
#include <stdint.h>
#include <string>
#include <vector>
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

/* @brief current game scene mode. */
enum GameSceneMode
{
  kGameSceneModeNone,
  kGameSceneModeLoading,
  kGameSceneModeMain,
  kGameSceneModeLogin,
  kGameSceneModeSelectMode,
  kGameSceneModeSelect,
  kGameSceneModeDecide,
  kGameSceneModePlay,
  kGameSceneModeResult,
  kGameSceneModeCourseResult,
  kGameSceneModeSetting,
  kGameSceneModeKeySetting /* Special mode, only for LR2 */,
  kGameSceneModeTest /* Special mode, only for testing */,
  kGameSceneClose /* End of a game, Unexitable game state. */,
};

/**
 * @brief
 * Contains running status of game, including settings.
 */
class Game
{
public:
  static Game& getInstance();
  static Setting& getSetting();
  bool Load();
  bool Save();
  void Default();
  void LoadOrDefault();

  /* Update game status. */
  void Update();

  /* Set next game mode */
  void SetNextScene(GameSceneMode next_game_mode);

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
  GameSceneMode get_game_scene_mode() const;
  Setting& get_game_setting();

  void set_do_logging(bool v);

private:
  Game();
  ~Game();

  static Game game_;

  // setting of game context
  Setting setting_;

  std::string setting_path_;

  uint16_t width_, height_;
  std::string log_path_;
  bool do_logging_;

  // misc attributes
  std::map<std::string, std::string> attributes_;

  // current game boot mode.
  GameBootMode game_boot_mode_;

  // current game scene.
  GameSceneMode game_scene_;

  // reserved next game scene.
  GameSceneMode next_game_scene_;

  void InitializeGameSceneMode();
  void InitializeGameOption();
  void ApplyGameOption();
};

}