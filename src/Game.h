#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>

namespace rhythmus
{

/* @brief Current game boot mode. */
enum GameBootMode
{
  NORMAL,
  TEST, /* hidden boot mode - only for test purpose */
};

/**
 * @brief
 * Event types
 */
enum GameEventTypes
{
  kOnKeyDown,
  kOnKeyUp,
  kOnText,
  kOnCursorMove,
  kOnCursorClick,
  kOnJoystick,
};

/**
 * @brief
 * Event object which is cached internally in game object.
 */
struct GameEvent
{
  // absolute game time msec
  uint32_t time_msec;
  // delta game time from rendering time
  uint32_t delta_time_msec;
  // game event type
  int event_type;
  // params
  int params[4];
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

bool IsEventKeyPress(const GameEvent& e);
bool IsEventKeyUp(const GameEvent& e);
int GetKeycode(const GameEvent& e);

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

  void set_setting_path(const std::string& path);
  uint16_t get_window_width() const;
  uint16_t get_window_height() const;
  std::string get_window_title() const;
  std::string get_log_path() const;
  bool get_do_logging() const;
  float GetAspect() const;
  GameBootMode get_boot_mode() const;

  void set_do_logging(bool v);

  static void SendKeyDownEvent(int key);
  static void SendKeyUpEvent(int key);
  static void SendTextEvent(uint32_t codepoint);
  static void SendCursorMoveEvent(int x, int y);
  static void SendCursorClickEvent(int button);
  void SendEvent(const GameEvent& e);
  void SendEvent(GameEvent&& e);
  void ProcessEvent();

  double fps_;
private:
  Game();
  ~Game();
  bool ProcessSystemEvent(const GameEvent& e);

  static Game game_;

  std::string setting_path_;
  uint16_t width_, height_;
  std::string log_path_;
  bool do_logging_;

  // lock used when using cached_events_
  std::mutex mtx_swap_lock;

  // cached events.
  // stored from input thread, flushed in rendering thread.
  std::vector<GameEvent> cached_events_;

  // current game boot mode.
  GameBootMode game_boot_mode_;

  // game theme related variables.
  GameThemeOption theme_option_;
};

}