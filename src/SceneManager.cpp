#include "SceneManager.h"
#include "scene/TestScene.h"
#include "scene/SelectScene.h"
#include "LR2/LR2SceneLoader.h"
#include "LR2/LR2Flag.h"

namespace rhythmus
{

SceneManager::SceneManager()
  : current_scene_(nullptr)
{

}

SceneManager::~SceneManager()
{
  if (current_scene_)
  {
    current_scene_->CloseScene();
    delete current_scene_;
  }
}

void SceneManager::Initialize()
{
  // TODO: load scenemanager(global scene) settings if necessary.

}

void SceneManager::Render()
{
  // Tick all scenemanager related timer
  timer_scene_.Tick();

  // Update LR2Flag (LR2 compatible layer)
  LR2Flag::Update();

  if (current_scene_)
    current_scene_->Render();
}

void SceneManager::SendEvent(const GameEvent& e)
{
  if (current_scene_)
    current_scene_->ProcessEvent(e);
}

void SceneManager::ChangeScene(bool force)
{
  // create scene suitable for current game mode
  Scene *new_scene = CreateNextScene();

  // if same scene, don't change scene
  if (current_scene_ && *current_scene_ == *new_scene && !force)
  {
    delete new_scene;
    return;
  }

  // load scene
  // TODO: use general SceneLoader class
  // TODO: TestScene won't need SceneLoader. need to take care of it.
  SceneLoader *scene_loader = new LR2SceneLoader(new_scene);
  scene_loader->Load("../themes/WMIX_HD/select/select.lr2skin");
  new_scene->LoadScene(scene_loader);
  delete scene_loader;

  delete current_scene_;
  current_scene_ = new_scene;

  // reset related timer
  timer_fadeout_.Stop();
  timer_scene_.Start();

  current_scene_->StartScene();
}

SceneManager& SceneManager::getInstance()
{
  static SceneManager scenemanager_;
  return scenemanager_;
}

Scene* SceneManager::CreateNextScene()
{
  auto mode = Game::getInstance().get_game_mode();

  // if test mode, then directly go into test scene
  if (mode == GameMode::kGameModeTest)
    return new TestScene();
  
  switch (mode)
  {
  case GameMode::kGameModeSelect:
    // TODO: LR2Scene or other type of scene?
    // LR2Scene is really necessary?
    return new SelectScene();
  case GameMode::kGameModeNone:
    /* return nullptr, which indicates not to process anything */
    Graphic::getInstance().ExitRendering();
    return nullptr;
  default:
    // NOT IMPLEMENTED or WRONG VALUE
    ASSERT(0);
  }

  // NOT REACHABLE.
  return nullptr;
}


Timer& SceneManager::GetSceneTimer()
{
  return timer_scene_;
}

}