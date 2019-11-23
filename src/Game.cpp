#include "Game.h"
#include "Graphic.h"
#include "Timer.h"
#include "Logger.h"
#include "Util.h"
#include "Song.h"
#include "SceneManager.h"
#include "Sound.h"
#include "Player.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "LR2/LR2Flag.h"  // For updating LR2 flag

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
  : is_running_(false), game_boot_mode_(GameBootMode::kBootNormal)
{
}

Game::~Game()
{
}

void Game::Initialize()
{
  // before starting initialization, system path should be cached first.
  ResourceManager::CacheSystemDirectory();

  // load settings before logging.
  Setting::ReadAll();

  // Start logging.
  Logger::getInstance().StartLogging();
  Logger::getInstance().HookStdOut();

  // initialize threadpool / graphic / sound
  TaskPool::getInstance().SetPoolSize(4);
  Graphic::Initialize();
  SoundDriver::getInstance().Initialize();

  // initialize all other elements ...
  EventManager::Initialize();
  Timer::Initialize();
  Player::Initialize();
  SceneManager::getInstance().Initialize();

  getInstance().is_running_ = true;
}

void Game::Loop()
{
  while (getInstance().is_running_ && !Graphic::IsWindowShouldClose())
  {
    /* Update main timer in graphic(main) thread. */
    Timer::Update();

    /**
     * Process cached events in main thread.
     * COMMENT: Event must be processed before Update() method
     * (e.g. Wheel flickering when items Rebuild after Event flush)
     */
    InputEventManager::Flush();
    EventManager::Flush();

    ResourceManager::UpdateMovie();

    Graphic::Render();
  }
}

/* @warn do not call this method directly! call Exit() instead. */
void Game::Cleanup()
{
  Player::Cleanup();
  TaskPool::getInstance().ClearTaskPool();
  SceneManager::getInstance().Cleanup();
  SongResource::getInstance().Clear();
  Graphic::getInstance().Cleanup();
  SoundDriver::getInstance().Destroy();
  Setting::SaveAll();
  Logger::getInstance().FinishLogging();
}

/* @warn this method does not exits game instantly;
 * exit after current game loop. */
void Game::Exit()
{
  getInstance().is_running_ = false;
  Graphic::SignalWindowClose();
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

const std::string &Game::get_window_title()
{
  static std::string name("Rhythmus 190800");
  return name;
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

}