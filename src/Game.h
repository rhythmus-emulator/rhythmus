#pragma once
#include <stdint.h>
#include <string>

namespace rhythmus
{

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

  void ProcessEvent();

  double fps_;
private:
  Game();
  ~Game();
  static Game game_;

  std::string setting_path_;
  uint16_t width_, height_;
  std::string log_path_;
  bool do_logging_;
};

}