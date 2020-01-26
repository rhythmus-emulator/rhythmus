#include "Game.h"
#include "Graphic.h"
#include "Timer.h"
#include "Logger.h"
#include "Util.h"
#include "Song.h"
#include "SongPlayer.h"
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

const char* const sGamemode[] = {
  "none",
  "4key",
  "5key",
  "6key",
  "7key",
  "8key",
  "IIDXSP",
  "IIDXDP",
  "IIDX5key",
  "IIDX10key",
  "popn",
  "ez2dj",
  "ddr",
  0
};

const char* GamemodeToString(int v)
{
  if (v >= Gamemode::kGamemodeEnd)
    return nullptr;
  return sGamemode[v];
}

int StringToGamemode(const char* s)
{
  auto ss = sGamemode;
  while (*ss)
  {
    if (stricmp(s, *ss) == 0)
      return ss - sGamemode;
    ++ss;
  }
  return Gamemode::kGamemodeNone;
}


/* --------------------------------- class Game */

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
  Logger::Initialize();

  // initialize threadpool / graphic / sound
  TaskPool::getInstance().SetPoolSize(4);
  Graphic::Initialize();
  SoundDriver::getInstance().Initialize();

  // initialize all other elements ...
  EventManager::Initialize();
  Timer::Initialize();
  PlayerManager::Initialize();
  SceneManager::getInstance().Initialize();

  getInstance().is_running_ = true;
}

void Game::Loop()
{
  try
  {
    while (getInstance().is_running_ && !Graphic::IsWindowShouldClose())
    {
      /* Update main timer in graphic(main) thread. */
      Timer::Update();

      /* TODO: Update Logger to log fps information. */

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
  catch (const RuntimeException &e)
  {
    /* log for unexcepted runtime exception (mostly fatal) and exit */
    Logger::Error("Fatal error (%s) : %s", e.exception_name(), e.what());
    Exit();
  }
  catch (const std::exception &e)
  {
    /* log for unknown exception and exit */
    Logger::Error("Fatal error (unknown) : %s", e.what());
    Exit();
  }
}

/* @warn do not call this method directly! call Exit() instead. */
void Game::Cleanup()
{
  SongPlayer::getInstance().Stop();
  PlayerManager::Cleanup();
  TaskPool::getInstance().ClearTaskPool();
  SceneManager::getInstance().Cleanup();
  Graphic::getInstance().Cleanup();
  SoundDriver::getInstance().Destroy();
  Setting::SaveAll();
  ResourceManager::getInstance().Cleanup();
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
    SongPlayer::getInstance().SetSongtoPlay(v, "");
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

}