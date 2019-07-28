#include "Game.h"
#include "Timer.h"
#include "Logger.h"
#include "tinyxml2.h"
#include <iostream>

namespace rhythmus
{

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Game::Game()
  : fps_(0)
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
  return atoi(v);
}

template<>
float GetValueConverted(const char* v)
{
  return static_cast<float>(atof(v));
}

template<>
double GetValueConverted(const char* v)
{
  return atof(v);
}

template<>
bool GetValueConverted(const char* v)
{
  return stricmp(v, "true") == 0;
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
  XML_SETTINGS;
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
  XML_SETTINGS;
#undef XMLATTR

  doc.LinkEndChild(settings);
  return doc.SaveFile(setting_path_.c_str()) == XML_SUCCESS;
}

void Game::Default()
{
  setting_path_ = "../config/config.xml";
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
  return "Rhythmus 190700";
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


void Game::set_do_logging(bool v)
{
  do_logging_ = v;
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

void Game::ProcessEvent()
{
  // TODO: process cached events.

  // Update FPS for internal use.
  FpsTimer.Tick();
}

}