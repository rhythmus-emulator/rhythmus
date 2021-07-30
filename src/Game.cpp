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
#include "config.h"
#include <thread>

#if USE_GLFW == 1
# include <GLFW/glfw3.h>
#endif

namespace rhythmus
{

Game *GAME = nullptr;

std::thread::id main_thread_id;

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
std::map<std::string, std::string> gPrefTemp;

/* --------------------------------- class Game */

Game::Game()
  : handler_(nullptr), is_running_(false), is_paused_(false),
    game_boot_mode_(GameBootMode::kBootNormal)
{
}

Game::~Game()
{
}

void Game::Create()
{
  R_ASSERT_M(GAME == nullptr, "Attempt to re-initialize GAME object!");
  GAME = new Game();
}

void Game::Initialize()
{
  // call Create() first.
  R_ASSERT(GAME);

  // thread id initialize
  main_thread_id = std::this_thread::get_id();

  // before starting initialization, path must be initialized
  PathCache::Initialize();

  // load settings before logging.
  Setting::Initialize();

  // if LR2 compatibility mode is on
  if (true/* TODO */) {
    PathCache::AddLR2SymLink();
  }

  // resource pool init & set system font
  ResourceManager::Initialize();
  FontManager::SetSystemFont();

  // flush temporary variables into setting.
  for (auto i : gMetricTemp)
    METRIC->set(i.first, i.second);
  for (auto i : gPrefTemp)
    PREFDATA->set(i.first, i.second);
  gMetricTemp.clear();
  gPrefTemp.clear();

  // Start logging.
  Logger::getInstance().Initialize();

  // Now we can initialize game handler.
  GAME->InitializeHandler();

  // initialize threadpool / graphic / sound
  TaskPool::Initialize();
  Graphic::CreateGraphic();
  TextureLoader::Initialize();
  SoundDriver::getInstance().Initialize();

  // initialize all other modules ...
  EventManager::Initialize();
  Timer::Initialize();
  PlayerManager::Initialize();
  SceneManager::Initialize();
  SongList::Initialize();

  GAME->is_running_ = true;
}

void Game::Loop()
{
  while (GAME->is_running_ && !GRAPHIC->IsWindowShouldClose())
  {
    /* Update main timer in graphic(main) thread. */
    //double inputdelta = Timer::SystemTimer().GetDeltaTime() * 1000;
    //double delta = GRAPHIC->delta() * 1000.0;

    try
    {
      /**
        * Process cached events in main thread.
        * COMMENT: Event must be processed before Update() method
        * (e.g. Wheel flickering when items Rebuild after Event flush)
        */
      if (GRAPHIC->IsVsyncUpdatable()) {
        Timer::Update();
        InputEventManager::Flush();
        EVENTMAN->Flush();

        /* Song and movie won't be updated if paused. */
        if (!GAME->IsPaused())
        {
          double delta = Timer::SystemTimer().GetDeltaTime() * 1000;
          SongPlayer::getInstance().Update((float)delta);
          ResourceManager::Update(delta);
        }

        /* Scene update & rendering */
        SCENEMAN->Update();
        TEXLOADER->Update();
        GRAPHIC->BeginFrame();
        SCENEMAN->Render();
        GRAPHIC->EndFrame();

        /* Flush enqueued task */
        TASKMAN->Update();

        /* Flush stdout into log message */
        Logger::getInstance().Flush();
      }
      else {
        // TODO: remove this part, and use dynamic async rendering
        std::this_thread::sleep_for(std::chrono::microseconds(10));
      }

      GAME->PollEvent();
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
  // Stop all working tasks first
  TASKMAN->AbortAllTask();

  // Delete all other elements
  SongPlayer::getInstance().Stop();
  SongList::Cleanup();
  PlayerManager::Cleanup();
  SceneManager::Cleanup();
  TaskPool::Destroy();
  TextureLoader::Destroy();
  Graphic::DeleteGraphic();
  SoundDriver::getInstance().Destroy();
  Setting::Save();
  Setting::Cleanup();
  ResourceManager::Cleanup();
  EventManager::Cleanup();
  Logger::getInstance().FinishLogging();

  GAME->DestroyHandler();
  delete GAME;
  GAME = nullptr;
}

/* @warn this method does not exits game instantly;
 * exit after current game loop. */
void Game::Exit()
{
  GAME->is_running_ = false;
  GRAPHIC->SignalWindowClose();
}

void Game::PollEvent()
{
  glfwPollEvents();
}

void Game::InitializeHandler()
{
#ifdef USE_GLFW
#endif
}

void Game::DestroyHandler()
{
#ifdef USE_GLFW
  if (handler_)
  {
    glfwDestroyWindow((GLFWwindow*)handler_);
    handler_ = 0;
  }
  glfwTerminate();
#endif
}

void Game::CenterWindow()
{
  GLFWwindow *window = (GLFWwindow*)handler_;
  int totalMonitor;
  GLFWmonitor** monitors = glfwGetMonitors(&totalMonitor);
  GLFWmonitor* monitor;
  if (!totalMonitor || !monitors) return;
  monitor = monitors[0];

  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
  if (!mode)
    return;

  int monitorX, monitorY;
  glfwGetMonitorPos(monitor, &monitorX, &monitorY);

  int windowWidth, windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);

  glfwSetWindowPos(window,
    monitorX + (mode->width - windowWidth) / 2,
    monitorY + (mode->height - windowHeight) / 2);
}

void *Game::handler() { return handler_; }
void Game::set_handler(void *p) { handler_ = p; }

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

  if (cmd == "--test") {
    game_boot_mode_ = GameBootMode::kBootTest;
    gPrefTemp["TestScene"] = v;
  }
  else if (cmd == "--reset") {
  } // TODO
  else if (cmd == "--reloadsong") {
  } // TODO
  else if (cmd == "--play") {
    game_boot_mode_ = GameBootMode::kBootPlay;
    SongPlayer::getInstance().SetSongtoPlay(v, "");
  }
  else if (cmd.substr(0, 2) == "-D") {
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

void Game::PopupMessage(const std::string &text)
{
  MessageBoxData::Add(MessageBoxTypes::kMbPopup, std::string(), text);
}

void Game::SetClipBoard(const std::string &s)
{
#ifdef USE_GLFW
  if (handler_)
  {
    glfwSetClipboardString((GLFWwindow*)handler_, s.c_str());
  }
#endif
}

void Game::Pause(bool pause)
{
  is_paused_ = pause;
}

bool Game::IsPaused()
{
  return is_paused_;
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

bool Game::is_main_thread()
{
  return main_thread_id == std::this_thread::get_id();
}

}
