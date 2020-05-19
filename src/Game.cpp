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
#include "LR2/LR2Flag.h"        // for updating LR2 flag
#include "scene/OverlayScene.h" // for invoke MessageBox
#include "common.h"

#include <GLFW/glfw3.h>

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

// temporary stored metric before initialization
std::map<std::string, std::string> gMetricTemp;

// temporary stored preference before initialization
std::map<std::string, int> gPrefIntTemp;
std::map<std::string, std::string> gPrefStrTemp;
std::map<std::string, std::string> gPrefFileTemp;

/* --------------------------------- class Game */

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Game::Game()
  : is_running_(false), is_paused_(false), game_boot_mode_(GameBootMode::kBootNormal)
{
}

Game::~Game()
{
}

void Game::Initialize()
{
  // before starting initialization, resource must be initialized
  ResourceManager::Initialize();

  // load settings before logging.
  Setting::Initialize();

  // flush temporary variables into setting.
  for (auto i : gMetricTemp)
    METRIC->set(i.first, i.second);
  for (auto i : gPrefIntTemp)
    PREFERENCE->SetInt(i.first, i.second);
  for (auto i : gPrefStrTemp)
    PREFERENCE->SetString(i.first, i.second);
  for (auto i : gPrefFileTemp)
    PREFERENCE->SetFile(i.first, i.second);
  gMetricTemp.clear();

  // Start logging.
  Logger::getInstance().Initialize();

  // initialize threadpool / graphic / sound
  TaskPool::getInstance().SetPoolSize(4);
  Graphic::CreateGraphic();
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
  while (getInstance().is_running_ && !GRAPHIC->IsWindowShouldClose())
  {
    /* Update main timer in graphic(main) thread. */
    Timer::Update();
    double delta = Timer::SystemTimer().GetDeltaTime() * 1000;

    try
    {
      /**
        * Process cached events in main thread.
        * COMMENT: Event must be processed before Update() method
        * (e.g. Wheel flickering when items Rebuild after Event flush)
        */
      InputEventManager::Flush();
      EventManager::Flush();

      /* Song and movie won't be updated if paused. */
      if (!Game::IsPaused())
      {
        SongPlayer::getInstance().Update(delta);
        ResourceManager::Update(delta);
      }

      /* Scene update & rendering */
      SceneManager::getInstance().Update();
      GRAPHIC->BeginFrame();
      SceneManager::getInstance().Render();
      GRAPHIC->EndFrame();

      /* Flush stdout into log message */
      Logger::getInstance().Flush();
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
}

/* @warn do not call this method directly! call Exit() instead. */
void Game::Cleanup()
{
  SongPlayer::getInstance().Stop();
  PlayerManager::Cleanup();
  SceneManager::getInstance().Cleanup();
  TaskPool::getInstance().ClearTaskPool();
  Graphic::DeleteGraphic();
  SoundDriver::getInstance().Destroy();
  Setting::Save();
  Setting::Cleanup();
  ResourceManager::Cleanup();
  Logger::getInstance().FinishLogging();
}

/* @warn this method does not exits game instantly;
 * exit after current game loop. */
void Game::Exit()
{
  getInstance().is_running_ = false;
  GRAPHIC->SignalWindowClose();
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
  {
    game_boot_mode_ = GameBootMode::kBootTest;
    gPrefFileTemp["TestScene"] = v;
  }
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
  else if (cmd.substr(0, 2) == "-D")
  {
    cmd = cmd.substr(2);
    gMetricTemp[cmd] = v;
  }
}

void Game::MessageBox(const std::string &title, const std::string &text)
{
  MessageBoxData::Add(MessageBoxTypes::kMbNone, title, text);
}

void Game::AlertMessageBox(const std::string &title, const std::string &text)
{
  MessageBoxData::Add(MessageBoxTypes::kMbInformation, title, text);
}

void Game::WarningMessageBox(const std::string &title, const std::string &text)
{
  MessageBoxData::Add(MessageBoxTypes::kMbWarning, title, text);
}

void Game::CriticalMessageBox(const std::string &title, const std::string &text)
{
  MessageBoxData::Add(MessageBoxTypes::kMbCritical, title, text);
}

void Game::Pause(bool pause)
{
  getInstance().is_paused_ = pause;
}

bool Game::IsPaused()
{
  return getInstance().is_paused_;
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
