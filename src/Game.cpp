#include "Game.h"
#include "Timer.h"
#include "Logger.h"
#include "tinyxml2.h"
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

Game::Game()
  : fps_(0), setting_path_(kSettingPath),
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
  using namespace tinyxml2;
  XMLDocument doc;
  XMLError errcode;
  if ((errcode = doc.LoadFile(setting_path_.c_str())) != XML_SUCCESS)
  {
    std::cerr << "Game settings reading failed, TinyXml2 ErrorCode: " << errcode << std::endl;
    return false;
  }
  XMLElement* settings = doc.RootElement();

  // Set default value - in case of key is not existing.
  Default();

#define XMLATTR(var, attrname, type) \
{ XMLElement *e = settings->FirstChildElement(attrname); \
  if (e) { var = GetValueConverted<type>(e->GetText()); } }
#define XMLATTRS(a,b,c) XMLATTR(a,b,c)
  XML_SETTINGS;
#undef XMLATTRS
#undef XMLATTR

  return true;
}

bool Game::Save()
{
  using namespace tinyxml2;
  XMLDocument doc;
  XMLElement* settings = doc.NewElement("Settings");

#define XMLATTR(var, attrname, type) \
{ XMLElement* e = doc.NewElement(attrname); e->SetText(var); settings->LinkEndChild(e); }
#define XMLATTRS(a,b,c) XMLATTR(a.c_str(),b,c)
  XML_SETTINGS;
#undef XMLATTRS
#undef XMLATTR

  doc.LinkEndChild(settings);
  return doc.SaveFile(setting_path_.c_str()) == XML_SUCCESS;
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

void Game::SendKeyDownEvent(int scancode)
{
  Game::getInstance().SendEvent({
    static_cast<uint32_t>(Timer::GetUncachedGameTime() * 1000), 0,
    GameEventTypes::kOnKeyDown,
    { scancode, 0, }
    });
}

void Game::SendKeyUpEvent(int scancode)
{
  Game::getInstance().SendEvent({
    static_cast<uint32_t>(Timer::GetUncachedGameTime() * 1000), 0,
    GameEventTypes::kOnKeyUp,
    { scancode, 0, }
    });
}

void Game::SendTextEvent(uint32_t codepoint)
{
  static_assert(sizeof(uint32_t) == sizeof(int),
    "Uint32_t and int size should be same. If not in some platform, You should fix to split codepoint into two params.");

  Game::getInstance().SendEvent({
    static_cast<uint32_t>(Timer::GetUncachedGameTime() * 1000), 0,
    GameEventTypes::kOnText,
    { static_cast<int>(codepoint), }
    });
}

void Game::SendCursorMoveEvent(int x, int y)
{
}

void Game::SendCursorClickEvent(int button)
{
}

void Game::SendEvent(const GameEvent& e)
{
  mtx_swap_lock.lock();
  cached_events_.push_back(e);
  mtx_swap_lock.unlock();
}

void Game::SendEvent(GameEvent&& e)
{
  mtx_swap_lock.lock();
  cached_events_.emplace_back(e);
  mtx_swap_lock.unlock();
}

// Timer for calculating FPS
class FPSTimer : public Timer
{
public:
  FPSTimer()
  {
    SetEventInterval(5, true);
  }

  virtual void OnEvent()
  {
    Game::getInstance().fps_ = GetTickRate();
    Logger::Info("FPS: %.2lf", Game::getInstance().fps_);
  }
} FpsTimer;

/* Size of pre-reserved for event cache (to improve performance) */
constexpr int kPreCacheEventCount = 64;

/* Process cached events in Rendering Thread. */
void Game::ProcessEvent()
{
  Game& g = Game::getInstance();

  // process cached events.
  // swap cache to prevent event lagging while ProcessEvent.
  // Pre-reserve enough space for new event cache to improve performance.
  std::vector<GameEvent> evnt(kPreCacheEventCount);
  mtx_swap_lock.lock();
  g.cached_events_.swap(evnt);
  mtx_swap_lock.unlock();

  for (const auto& e : evnt)
  {
    if (!g.ProcessSystemEvent(e)) continue;

    // Send remain event to SceneManager.
    SceneManager::getInstance().SendEvent(e);
  }

  // Update FPS for internal use.
  FpsTimer.Tick();
}

/**
 * @brief Process events which should be processed in global level.
 * return value: false means don't propagate event to scene.
 */
bool Game::ProcessSystemEvent(const GameEvent& e)
{
  return true;
}



// Utility functions

bool IsEventKeyPress(const GameEvent& e)
{
  return (e.event_type == GameEventTypes::kOnKeyDown);
}

bool IsEventKeyUp(const GameEvent& e)
{
  return (e.event_type == GameEventTypes::kOnKeyUp);
}

int GetKeycode(const GameEvent& e)
{
  return e.params[0];
}

}