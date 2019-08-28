#include "Game.h"
#include "Timer.h"
#include "Logger.h"
#include "SceneManager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

namespace rhythmus
{

constexpr const char* kSettingPath = "../config/config.xml";

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Setting& Game::getSetting()
{
  return game_.setting_;
}

Game::Game()
  : setting_path_(kSettingPath),
    game_boot_mode_(GameBootMode::kBootNormal),
    game_mode_(GameMode::kGameModeNone),
    do_game_mode_change_(false)
{
}

Game::~Game()
{
}

/* macro to deal with save/load properties */
/* (var_name, xml_attr_name, type, default value) */
#define XML_SETTINGS \
  XMLATTR(width_, "width", uint16_t) \
  XMLATTR(height_, "height", uint16_t) \
  XMLATTR(do_logging_, "logging", bool) \
  XMLATTRS(theme_option_.scene_path, "scene_path", std::string) \
  XMLATTRS(theme_option_.select_scene_path, "select_scene_path", std::string) \
  XMLATTRS(theme_option_.decide_scene_path, "decide_scene_path", std::string) \
  XMLATTRS(theme_option_.play_scene_path, "play_scene_path", std::string) \
  XMLATTRS(theme_option_.result_path, "result_path", std::string) \
  XMLATTRS(theme_option_.courseresult_scene_path, "courseresult_scene_path", std::string) \

template<typename A>
A GetValueConverted(const char* v);

template<>
uint16_t GetValueConverted(const char* v)
{
  return static_cast<uint16_t>(atoi(v));
}

template<>
int GetValueConverted(const char* v)
{
  if (!v) return 0;
  return atoi(v);
}

template<>
float GetValueConverted(const char* v)
{
  if (!v) return 0.f;
  return static_cast<float>(atof(v));
}

template<>
double GetValueConverted(const char* v)
{
  if (!v) return .0;
  return atof(v);
}

template<>
bool GetValueConverted(const char* v)
{
  if (!v) return false;
  return stricmp(v, "true") == 0;
}

template<>
std::string GetValueConverted(const char* v)
{
  if (!v) return "";
  return std::string(v);
}

bool Game::Load()
{
  // Set default value - in case of key is not existing.
  Default();

  if (!setting_.Open(setting_path_))
  {
    std::cerr << "Failed to game load settings, use default value." << std::endl;
    return false;
  }

#define XMLATTR(var, attrname, type) \
setting_.Load(attrname, var);
#define XMLATTRS(a,b,c) XMLATTR(a,b,c)
  XML_SETTINGS;
#undef XMLATTRS
#undef XMLATTR

  return true;
}

bool Game::Save()
{
#define XMLATTR(var, attrname, type) \
setting_.Set(attrname, var);
#define XMLATTRS(a,b,c) XMLATTR(a,b,c)
  XML_SETTINGS;
#undef XMLATTRS
#undef XMLATTR

  return setting_.Save();
}

void Game::Default()
{
  setting_path_ = kSettingPath;
  width_ = 640;
  height_ = 480;
  log_path_ = "../log/log.txt";
  do_logging_ = false;
}

void Game::LoadOrDefault()
{
  if (!Load())
  {
    std::cerr << "Failed to load game setting file. use default settings." << std::endl;
    Default();
  }
}

void Game::Update()
{
  // Tick all game timers
  // TODO

  // Check scene should need to be changed
  if (do_game_mode_change_)
  {
    if (next_game_mode_ != GameMode::kGameModeNone)
      /* if preserved next game mode exists, use it. */
      game_mode_ = next_game_mode_;
    else
    {
      switch (game_boot_mode_)
      {
      case GameBootMode::kBootTest:
        if (game_mode_ == GameMode::kGameModeNone)
          game_mode_ = GameMode::kGameModeTest;
        else
          game_mode_ = GameMode::kGameClose;
        break;
      case GameBootMode::kBootNormal:
        switch (game_mode_)
        {
        case GameMode::kGameModeNone:
          // We now need to decide whether start game in select scene -
          // or in main scene. It is decided by option... If main scene
          // is null, we start game in select scene.
          if (!theme_option_.scene_path.empty())
            game_mode_ = GameMode::kGameModeMain;
          else
            game_mode_ = GameMode::kGameModeSelect;
          break;
        case GameMode::kGameModeMain:
          game_mode_ = GameMode::kGameModeSelectMode;
          break;
        case GameMode::kGameModeSelectMode:
          game_mode_ = GameMode::kGameModeSelect;
          break;
        case GameMode::kGameModeSelect:
          game_mode_ = GameMode::kGameModeDecide;
          break;
        case GameMode::kGameModeDecide:
          game_mode_ = GameMode::kGameModePlay;
          break;
        case GameMode::kGameModePlay:
          game_mode_ = GameMode::kGameModeResult;
          break;
        case GameMode::kGameModeResult:
          // TODO: need to go to course result if necessary.
          // TODO: need to go to Play scene again if course mode.
          // TODO: need to go to Main scene if it's arcade mode.
        case GameMode::kGameModeCourseResult:
          game_mode_ = GameMode::kGameModeSelect;
          break;
        }
        break;
      }
    }

    next_game_mode_ = GameMode::kGameModeNone;
    do_game_mode_change_ = false;
    SceneManager::getInstance().ChangeScene();
  }
}

void Game::SetNextGameMode(GameMode next_game_mode)
{
  next_game_mode_ = next_game_mode;
}

void Game::ChangeGameMode()
{
  do_game_mode_change_ = true;
}

template<>
std::string Game::GetAttribute(const std::string& key) const
{
  const auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    return ii->first;
  else return std::string();
}

void Game::SetAttribute(const std::string& key, const std::string& value)
{
  attributes_[key] = value;
}

void Game::set_setting_path(const std::string& path)
{
  setting_path_ = path;
}

uint16_t Game::get_window_width() const
{
  return width_;
}

uint16_t Game::get_window_height() const
{
  return height_;
}

std::string Game::get_window_title() const
{
  return "Rhythmus 190800";
}

std::string Game::get_log_path() const
{
  return log_path_;
}

bool Game::get_do_logging() const
{
  return do_logging_;
}

float Game::GetAspect() const
{
  return (float)width_ / height_;
}

GameBootMode Game::get_boot_mode() const
{
  return game_boot_mode_;
}

GameMode Game::get_game_mode() const
{
  return game_mode_;
}


void Game::set_do_logging(bool v)
{
  do_logging_ = v;
}

}