#include "Game.h"
#include <iostream>

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Game::Game()
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