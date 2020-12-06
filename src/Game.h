#pragma once

#include "Timer.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <map>

/**
 * Our game module
 * If we're going to support operating system that glfw does not support,
 * e.g. Android, we need to work again a lot ...
 */
 
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
  kGamemodeIIDX,
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
  static void Create();
  static void Initialize();
  static void Loop();
  static void Cleanup();
  static void Exit();

  /* Poll event in system level. Must be executed from main thread. */
  void PollEvent();

  void CenterWindow();

  /* Generally low-level API handler. */
  void *handler();
  void set_handler(void *p);

  /* Update game status. */
  void Update();

  /* Load execute argument */
  void LoadArgument(const std::string& argv);

  void MessageBox(const std::string &title, const std::string &text);
  void AlertMessageBox(const std::string &title, const std::string &text);
  void WarningMessageBox(const std::string &title, const std::string &text);
  void CriticalMessageBox(const std::string &title, const std::string &text);
  void PopupMessage(const std::string &text);
  void SetClipBoard(const std::string &s);
  void Pause(bool pause);
  bool IsPaused();

  GameBootMode get_boot_mode() const;
  static const std::string &get_window_title();
  bool is_main_thread() const;

private:
  Game();
  ~Game();

  void InitializeHandler();
  void DestroyHandler();

  // core object handler
  void *handler_;

  // is game loop running?
  bool is_running_;

  // is game paused? (due to dialog or something)
  bool is_paused_;

  // current game boot mode.
  GameBootMode game_boot_mode_;
};

extern Game *GAME;

}
