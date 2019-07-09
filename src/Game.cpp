#include "Game.h"

Game Game::game_;

Game& Game::getInstance()
{
  return game_;
}

Game::Game()
  : setting_path_("../config/config.json"),
  width_(640), height_(480)
{
}

Game::~Game()
{
}

bool Game::Load()
{
  return true;
}

bool Game::Save()
{
  return true;
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