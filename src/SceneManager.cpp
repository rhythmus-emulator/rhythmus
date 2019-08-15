#include "SceneManager.h"
#include "scene/TestScene.h"
#include "LR2/LR2Scene.h"

namespace rhythmus
{

SceneManager::SceneManager()
  : current_scene_(nullptr)
{

}

SceneManager::~SceneManager()
{
  current_scene_->CloseScene();
  delete current_scene_;
}

void SceneManager::Initialize()
{
  if (Game::getInstance().get_boot_mode() == GameBootMode::TEST)
    current_scene_ = new TestScene();
  else if (Game::getInstance().get_boot_mode() == GameBootMode::NORMAL)
    current_scene_ = new LR2Scene();
  current_scene_->LoadScene();
}

void SceneManager::Render()
{
  if (current_scene_)
    current_scene_->Render();
}

void SceneManager::SendEvent(const GameEvent& e)
{
  if (current_scene_)
    current_scene_->ProcessEvent(e);
}

SceneManager& SceneManager::getInstance()
{
  static SceneManager scenemanager_;
  return scenemanager_;
}

}