#include "Game.h"
#include "Timer.h"
#include "Logger.h"
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

bool Game::Load()
{
  return false;
}

bool Game::Save()
{
  return false;
}

void Game::Default()
{
  setting_path_ = "../config/config.json";
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