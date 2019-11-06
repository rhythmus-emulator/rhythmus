#include "SceneManager.h"
#include "scene/TestScene.h"
#include "scene/LoadingScene.h"
#include "scene/SelectScene.h"
#include "scene/DecideScene.h"
#include "scene/PlayScene.h"
#include "scene/ResultScene.h"
#include "LR2/LR2SceneLoader.h"
#include "LR2/LR2Flag.h"
#include "Util.h"
#include <iostream>

namespace rhythmus
{

// ------------------------- class SceneManager

SceneManager::SceneManager()
  : current_scene_(nullptr)
{
  // self subscription start
  SubscribeTo(Events::kOnKeyDown);
  SubscribeTo(Events::kOnKeyPress);
  SubscribeTo(Events::kOnKeyUp);
  SubscribeTo(Events::kEventSongLoadFinished);
  SubscribeTo(Events::kEventSongStarted);
}

SceneManager::~SceneManager()
{
  Cleanup();
}

void SceneManager::Initialize()
{
  // load scenemanager(global scene) settings.
  // XXX: is it necessary?
  if (!setting_.Open("../config/scene.xml"))
  {
    std::cerr << "Cannot open Scene preference file. use default value." << std::endl;
  }

  // load soundset.
  // TODO: load soundset file path setting from game
  std::string soundset_path = "../sound/lr2.lr2ss";
  LoadMetrics(soundset_path);

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
  metrics_list_.clear();
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
  timer_scene_.Start();

  current_scene_->StartScene();
}

Scene* SceneManager::get_current_scene()
{
  return getInstance().current_scene_;
}

Scene* SceneManager::CreateNextScene()
{
  auto mode = Game::getInstance().get_game_scene_mode();

  // if test mode, then directly go into test scene
  if (mode == GameSceneMode::kGameSceneModeTest)
    return new TestScene();
  
  switch (mode)
  {
  case GameSceneMode::kGameSceneModeLoading:
    return new LoadingScene();
  case GameSceneMode::kGameSceneModeSelect:
    return new SelectScene();
  case GameSceneMode::kGameSceneModeDecide:
    return new DecideScene();
  case GameSceneMode::kGameSceneModePlay:
    return new PlayScene();
  case GameSceneMode::kGameSceneModeResult:
    return new ResultScene();
  case GameSceneMode::kGameSceneModeNone:
    /* return nullptr, which indicates not to process anything */
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

ThemeMetrics *SceneManager::getMetrics(const std::string &name)
{
  auto &metrics = getInstance().metrics_list_;
  auto it = metrics.find(name);
  if (it == metrics.end()) return nullptr;
  return &it->second;
}

ThemeMetrics *SceneManager::createMetrics(const std::string &name)
{
  return &getInstance().metrics_list_[name];
}

}