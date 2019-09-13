#include "SceneManager.h"
#include "scene/TestScene.h"
#include "scene/LoadingScene.h"
#include "scene/SelectScene.h"
#include "scene/DecideScene.h"
#include "scene/PlayScene.h"
#include "scene/ResultScene.h"
#include "LR2/LR2SceneLoader.h"
#include "LR2/LR2Flag.h"
#include <iostream>

namespace rhythmus
{

SceneManager::SceneManager()
  : current_scene_(nullptr)
{
  // self subscription start
  SubscribeTo(Events::kOnKeyDown);
  SubscribeTo(Events::kOnKeyPress);
  SubscribeTo(Events::kOnKeyUp);
  SubscribeTo(Events::kEventSceneTimeEnd);
  SubscribeTo(Events::kEventSceneChange);
}

SceneManager::~SceneManager()
{
  Cleanup();
}

void SceneManager::Initialize()
{
  // load scenemanager(global scene) settings.
  if (!setting_.Open("../config/scene.xml"))
  {
    std::cerr << "Cannot open Scene preference file." << std::endl;
    return;
  }

  // create starting scene.
  ChangeScene();
}

void SceneManager::Cleanup()
{
  if (current_scene_)
  {
    current_scene_->CloseScene();
    delete current_scene_;
    current_scene_ = 0;

    // automatically save scene settings
    if (!setting_.Save())
    {
      std::cerr << "Cannot save Scene preference file." << std::endl;
    }
  }
}

void SceneManager::Update()
{
  // Tick all scenemanager related timer
  timer_scene_.Tick();

  // Update LR2Flag (LR2 compatible layer)
  LR2Flag::Update();

  if (current_scene_)
    current_scene_->Update(timer_scene_.GetDeltaTime() * 1000);
}

void SceneManager::Render()
{
  if (current_scene_)
    current_scene_->Render();
}

bool SceneManager::OnEvent(const EventMessage& e)
{
  switch (e.GetEventID())
  {
  case Events::kEventSceneChange:
    Game::getInstance().ChangeGameMode();
    break;
  case Events::kEventSceneTimeEnd:
    current_scene_->CloseScene();
    break;
  }

  if (current_scene_)
    return current_scene_->ProcessEvent(e);

  return true;
}

void SceneManager::ChangeScene(bool force)
{
  // create scene suitable for current game mode
  Scene *new_scene = CreateNextScene();
  Scene *prev_scene = current_scene_;

  // if same scene, don't change scene
  if (current_scene_ && *current_scene_ == *new_scene && !force)
  {
    delete new_scene;
    return;
  }

  // load scene first to maximize shared resource between two scene
  current_scene_ = new_scene;
  new_scene->LoadScene();

  // now delete previous scene
  delete prev_scene;

  // Reset SceneManager timer
  // Need to refresh whole timing to set exact scene start timing
  // As much time passed due to scene loading.
  Timer::Update();
  timer_fadeout_.Stop();
  timer_scene_.Start();

  current_scene_->StartScene();
}

Scene* SceneManager::get_current_scene()
{
  return getInstance().current_scene_;
}

Scene* SceneManager::CreateNextScene()
{
  auto mode = Game::getInstance().get_game_mode();

  // if test mode, then directly go into test scene
  if (mode == GameMode::kGameModeTest)
    return new TestScene();
  
  switch (mode)
  {
  case GameMode::kGameModeLoading:
    return new LoadingScene();
  case GameMode::kGameModeSelect:
    return new SelectScene();
  case GameMode::kGameModeDecide:
    return new DecideScene();
  case GameMode::kGameModePlay:
    return new PlayScene();
  case GameMode::kGameModeResult:
    return new ResultScene();
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
  return getInstance().timer_scene_;
}

uint32_t SceneManager::GetSceneTickTime()
{
  return getInstance().timer_scene_.GetDeltaTimeInMillisecond();
}

SceneManager& SceneManager::getInstance()
{
  static SceneManager scenemanager_;
  return scenemanager_;
}

Setting& SceneManager::getSetting()
{
  return getInstance().setting_;
}

}