#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>

namespace rhythmus
{

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
  // cached events
  std::vector<GameEvent> cached_events_;
};

}