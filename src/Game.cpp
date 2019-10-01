#include "Game.h"
#include "Timer.h"
#include "Logger.h"
#include "Util.h"
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
    game_scene_(GameSceneMode::kGameSceneModeNone)
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
  XMLATTR(do_logging_, "logging", bool)

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

  // before loading option, set constraint & initialize.
  InitializeGameOption();
  setting_.ValidateAll();

  // just load option values - not option node itself.
  if (!setting_.ReloadValues(setting_path_))
  {
    std::cerr << "Failed to game load settings, use default value." << std::endl;
    return false;
  }

  // apply settings into system environment
  ApplyGameOption();

  // set starting game scene mode
  InitializeGameSceneMode();

  return true;
}

bool Game::Save()
{
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


  /* change game mode. */
  if (next_game_scene_ != GameSceneMode::kGameSceneModeNone)
  {
    /* exiting status */
    if (next_game_scene_ == GameSceneMode::kGameSceneClose)
    {
      Graphic::getInstance().ExitRendering();
    }

    game_scene_ = next_game_scene_;
    next_game_scene_ = GameSceneMode::kGameSceneModeNone;
    SceneManager::getInstance().ChangeScene();
  }
}

void Game::SetNextScene(GameSceneMode next_game_mode)
{
  next_game_scene_ = next_game_mode;
}

void Game::LoadArgument(const std::string& argv)
{
  if (argv[0] != '-') return;
  if (argv == "-test")
    game_boot_mode_ = GameBootMode::kBootTest;
  else if (argv == "-reset")
  {
  } // TODO
  else if (argv == "-reloadsong")
  {
  } // TODO
}

template<>
std::string Game::GetAttribute(const std::string& key) const
{
  const auto ii = attributes_.find(key);
  if (ii != attributes_.end())
    return ii->second;
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

GameSceneMode Game::get_game_scene_mode() const
{
  return game_scene_;
}

Setting& Game::get_game_setting()
{
  return setting_;
}


void Game::set_do_logging(bool v)
{
  do_logging_ = v;
}

void Game::InitializeGameSceneMode()
{
  switch (game_boot_mode_)
  {
  case GameBootMode::kBootNormal:
  case GameBootMode::kBootArcade:
  case GameBootMode::kBootLR2:
  case GameBootMode::kBootRefresh:
    game_scene_ = GameSceneMode::kGameSceneModeLoading;
    break;
  case GameBootMode::kBootTest:
    game_scene_ = GameSceneMode::kGameSceneModeTest;
    break;
  }
}

void Game::InitializeGameOption()
{
  /* Initialize game setting constraints */
  {
    Option &option = *setting_.NewOption("Resolution");
    option.set_description("Game resolution.");
    // TODO: get exact information from graphic card
    option.SetOption("640x480,800x600,1280x960,1280x720,1440x900,1600x1050,1920x1200");
  }

  {
    Option &option = *setting_.NewOption("SoundDevice");
    option.set_description("Set default sound device.");
    option.SetOption("");   // empty
  }

  {
    Option &option = *setting_.NewOption("SoundBufferSize");
    option.set_description("Sound latency increases if sound buffer is big. If sound flickers, use large buffer size.");
    option.SetOption("1024,2048,3072,4096,8192,16384");
    option.set_value(2048); // default value
  }

  {
    Option &option = *setting_.NewOption("Volume");
    option.set_description("Set game volume.");
    option.SetOption("0,10,20,30,40,50,60,70,80,90,100");
  }

  {
    Option &option = *setting_.NewOption("GameMode");
    option.set_description("Set game mode, whether to run as arcade or home.");
    option.SetOption("Home,Arcade,LR2");
  }

  {
    Option &option = *setting_.NewOption("Logging");
    option.set_description("For development.");
    option.SetOption("Off,On");
  }

  {
    Option &option = *setting_.NewOption("SelectScene");
    option.set_description("File path of select scene.");
    option.SetFileOption("../themes/*/select/*.lr2skin");
  }

  {
    Option &option = *setting_.NewOption("DecideScene");
    option.set_description("File path of decide scene.");
    option.SetFileOption("../themes/*/decide/*.lr2skin");
  }

  {
    Option &option = *setting_.NewOption("PlayScene");
    option.set_description("File path of play scene.");
    option.SetFileOption("../themes/*/play/*.lr2skin");
  }

  {
    Option &option = *setting_.NewOption("ResultScene");
    option.set_description("File path of result scene.");
    option.SetFileOption("../themes/*/result/*.lr2skin");
  }

  {
    Option &option = *setting_.NewOption("CourseResultScene");
    option.set_description("File path of course result scene.");
    option.SetFileOption("../themes/*/courseresult/*.lr2skin");
  }
}

void Game::ApplyGameOption()
{
  {
    /* a little trick */
    std::string res;
    setting_.LoadOptionValue("Resolution", res);
    size_t res_sep = res.find('x');
    if (res_sep == std::string::npos)
    {
      std::cerr << "Invalid resolution value: " << res << std::endl;
      return;
    }
    width_ = atoi(res.c_str());
    height_ = atoi(res.c_str() + res_sep + 1);
  }

  setting_.LoadOptionValue("Logging", do_logging_);

#define SET_SCENE_PARAM(scenename) \
{ Option* o; if ((o = setting_.GetOption(scenename)))\
SetAttribute(scenename, o->value()); }

  SET_SCENE_PARAM("SelectScene");
  SET_SCENE_PARAM("DecideScene");
  SET_SCENE_PARAM("PlayScene");
  SET_SCENE_PARAM("ResultScene");
  SET_SCENE_PARAM("CourseResultScene");

#undef SET_SCENE_PARAM
}

}