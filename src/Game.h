#pragma once
#include <stdint.h>
#include <string>

/**
 * @brief
 * Contains running status of game.
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
  float GetAspect() const;

private:
  Game();
  ~Game();
  static Game game_;

  std::string setting_path_;
  uint16_t width_, height_;
};