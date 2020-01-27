#pragma once

#include "Timer.h"
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

enum Gamemode
{
  kGamemodeNone,
  kGamemode4Key,
  kGamemode5Key,
  kGamemode6Key,
  kGamemode7Key,
  kGamemode8Key,
  kGamemodeIIDXSP,
  kGamemodeIIDXDP,
  kGamemodeIIDX5Key,
  kGamemodeIIDX10Key,
  kGamemodePopn,
  kGamemodeEZ2DJ,
  kGamemodeDDR,
  kGamemodeEnd,
};
const char* GamemodeToString(int gamemode);
int StringToGamemode(const char* s);

/**
 * @brief
 * Contains running status of game, including settings.
 */
class Game
{
public:
  static void Initialize();
  static void Loop();
  static void Cleanup();
  static void Exit();

  static Game& getInstance();

  /* Update game status. */
  void Update();

  /* Load execute argument */
  void LoadArgument(const std::string& argv);

  static void MessageBox(const std::string &title, const std::string &text);
  static void AlertMessageBox(const std::string &title, const std::string &text);
  static void WarningMessageBox(const std::string &title, const std::string &text);
  static void CriticalMessageBox(const std::string &title, const std::string &text);
  static void Pause(bool pause);
  static bool IsPaused();

  GameBootMode get_boot_mode() const;

  static const std::string &get_window_title();

private:
  Game();
  ~Game();

  static Game game_;

  // is game loop running?
  bool is_running_;

  // is game paused? (due to dialog or something)
  bool is_paused_;

  // current game boot mode.
  GameBootMode game_boot_mode_;
};

}