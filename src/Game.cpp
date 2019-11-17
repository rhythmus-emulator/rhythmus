#include "Game.h"
#include "Timer.h"
#include "Logger.h"
#include "Util.h"
#include "Song.h"
#include "SceneManager.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

namespace rhythmus
{

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Game::Game()
  : game_boot_mode_(GameBootMode::kBootNormal)
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

void Game::Default()
{
  width_ = 640;
  height_ = 480;
  log_path_ = "../log/log.txt";
  do_logging_ = false;
}

void Game::Update()
{
  // Tick all game timers
  // TODO
}

void Game::LoadArgument(const std::string& argv)
{
  std::string cmd;
  std::string v;
  if (argv[0] != '-') return;
  Split(argv, '=', cmd, v);

  if (cmd == "-test")
    game_boot_mode_ = GameBootMode::kBootTest;
  else if (cmd == "-reset")
  {
  } // TODO
  else if (cmd == "-reloadsong")
  {
  } // TODO
  else if (cmd == "-play")
  {
    game_boot_mode_ = GameBootMode::kBootPlay;

    // XXX: check whether a file is course or not?
    // XXX: what if file load failed?
    push_song(v);
    SongList::getInstance().LoadFileIntoSongList(v);
    SongList::getInstance().select(0);
  }
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

void Game::push_song(const std::string& songpath)
{
  song_queue_.push_back(songpath);
}

bool Game::pop_song(std::string& songpath)
{
  if (song_queue_.empty())
    return false;
  songpath = song_queue_.front();
  song_queue_.pop_front();
  return true;
}


void Game::set_do_logging(bool v)
{
  do_logging_ = v;
}

void Game::InitializeGameOption()
{
}

void Game::ApplyGameOption()
{
  /* TODO: move graphic related options to Graphic class. */
#if 0
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
#endif
}

}