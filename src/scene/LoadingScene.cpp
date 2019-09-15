#include "LoadingScene.h"
#include "Song.h"
#include <iostream>

namespace rhythmus
{

// singletons used for loading context

std::string current_loading_file;

// ------------------------- class LoadingScene

LoadingScene::~LoadingScene()
{
}

const std::string LoadingScene::GetSceneName() const
{
  return "LoadingScene";
}

void LoadingScene::LoadScene()
{
  sys_font_ = ResourceManager::GetSystemFont();
  message_text_.SetFont(sys_font_.get());
  current_file_text_.SetFont(sys_font_.get());


  message_text_.SetPos(
    320,
    Game::getInstance().get_window_height() - 260
  );
  current_file_text_.SetPos(
    320,
    Game::getInstance().get_window_height() - 200
  );
  loading_bar_.SetPos(
    240,
    Game::getInstance().get_window_height() - 200
  );

  // Register childs
  AddChild(&message_text_);
  AddChild(&current_file_text_);
  AddChild(&loading_bar_);
}

void LoadingScene::StartScene()
{
  SongList::getInstance().Load();
  message_text_.SetText("Song loading ...");
}

void LoadingScene::CloseScene()
{
}

bool LoadingScene::ProcessEvent(const EventMessage& e)
{
  if (e.IsKeyUp() && e.GetKeycode() == GLFW_KEY_ESCAPE)
  {
    // cancel all loading thread and directly exit game
    Graphic::getInstance().ExitRendering();
    return false;
  }
  else if (e.IsKeyUp() && !SongList::getInstance().is_loading())
  {
    // manually go to next scene
    Game::getInstance().SetNextGameMode(GameMode::kGameModeSelect);
    Game::getInstance().ChangeGameMode();
  }
  return true;
}

void LoadingScene::doUpdate(float)
{
  if (SongList::getInstance().is_loading())
  {
    std::string path = SongList::getInstance().get_loading_filename();
    int prog = static_cast<int>(SongList::getInstance().get_progress() * 100);
    sys_font_->PrepareText(path);
    sys_font_->Commit();
    message_text_.SetText("Loading " + std::to_string(prog) + "%");
    current_file_text_.SetText(path);
  }
  else
  {
    current_file_text_.Clear();
    message_text_.SetText("Ready ...!");
#if 0
    Game::getInstance().SetNextGameMode(GameMode::kGameModeSelect);
    Game::getInstance().ChangeGameMode();
#endif
  }
}

}