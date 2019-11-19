#include "SceneManager.h"
#include "scene/TestScene.h"
#include "scene/LoadingScene.h"
#include "scene/SelectScene.h"
#include "scene/DecideScene.h"
#include "scene/PlayScene.h"
#include "scene/ResultScene.h"
#include "LR2/LR2Flag.h"
#include "Util.h"
#include "Setting.h"
#include <iostream>

namespace rhythmus
{

// ------------------------- class SceneManager

SceneManager::SceneManager()
  : current_scene_(nullptr), next_scene_(nullptr)
{
  memset(visible_groups_, 0, sizeof(visible_groups_));
  visible_groups_[0] = 1;
}

SceneManager::~SceneManager()
{
  Cleanup();
}

void SceneManager::Initialize()
{
  // create starting scene.
  switch (Game::getInstance().get_boot_mode())
  {
  case GameBootMode::kBootNormal:
  case GameBootMode::kBootArcade:
  case GameBootMode::kBootLR2:
  case GameBootMode::kBootRefresh:
    SceneManager::ChangeScene("SceneLoading");
    break;
  case GameBootMode::kBootTest:
    SceneManager::ChangeScene("SceneTest");
    break;
  }
}

void SceneManager::Cleanup()
{
  if (current_scene_)
  {
    delete current_scene_;
    current_scene_ = 0;
  }
}

void SceneManager::Update()
{
  // Tick all scenemanager related timer
  timer_scene_.Tick();

  // check is it necessary to change scene
  // StartScene is called here (time critical process)
  if (next_scene_)
  {
    delete current_scene_;
    current_scene_ = next_scene_;
    next_scene_ = nullptr;

    current_scene_->StartScene();

    // Reset SceneManager timer
    // Need to refresh whole timing to set exact scene start timing
    // As much time passed due to scene loading.
    Timer::Update();
    timer_scene_.Start();
  }

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

void SceneManager::OnInputEvent(const InputEvent& e)
{
  if (current_scene_)
    current_scene_->ProcessInputEvent(e);
}

Scene* SceneManager::get_current_scene()
{
  return getInstance().current_scene_;
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

int SceneManager::getVisible(size_t index)
{
  if (index >= 1000) return;
  return getInstance().visible_groups_[index];
}

void SceneManager::setVisible(size_t index, int value)
{
  if (index >= 1000) return;
  getInstance().visible_groups_[index] = value;
}

void SceneManager::ChangeScene(const std::string &scene_name)
{
  static std::map <std::string, std::function<Scene*()> > sceneCreateFn;
  if (sceneCreateFn.empty())
  {
    sceneCreateFn["SceneTest"] = []() { return new TestScene(); };
    sceneCreateFn["SceneLoading"] = []() { return new LoadingScene(); };
    sceneCreateFn["SceneSelect"] = []() { return new SelectScene(); };
    sceneCreateFn["SceneDecide"] = []() { return new DecideScene(); };
    sceneCreateFn["ScenePlay"] = []() { return new PlayScene(); };
    sceneCreateFn["SceneResult"] = []() { return new ResultScene(); };
    //sceneCreateFn["SceneCourseResult"] = []() { return new CourseResultScene(); };
  }

  auto &inst = getInstance();
  bool is_exit = false;

  if (inst.next_scene_)
  {
    std::cout << "Warning: Next scene is already set & cached." << std::endl;
    return;
  }

  if (scene_name.empty() || scene_name == "exit")
    is_exit = true;

  auto it = sceneCreateFn.find(scene_name);
  if (it == sceneCreateFn.end())
    is_exit = true;

  if (is_exit)
  {
    // prepare to exit game
    Game::Exit();
  }

  inst.next_scene_ = it->second();

  // LoadScene is done here
  // (time-consuming loading is done which is not time critical
  //  e.g. Texture loading)
  inst.next_scene_->LoadScene();
}

}