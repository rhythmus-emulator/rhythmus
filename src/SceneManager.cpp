#include "SceneManager.h"
#include "scene/TestScene.h"
#include "scene/LoadingScene.h"
#include "scene/SelectScene.h"
#include "scene/DecideScene.h"
#include "scene/PlayScene.h"
#include "scene/ResultScene.h"
#include "scene/OverlayScene.h"
#include "LR2/LR2Flag.h"
#include "Util.h"
#include "Setting.h"
#include "common.h"

namespace rhythmus
{

// ------------------------- class SceneManager

SceneManager::SceneManager()
  : current_scene_(nullptr), background_scene_(nullptr), next_scene_(nullptr)
{
}

SceneManager::~SceneManager()
{
  Cleanup();
}

void SceneManager::Initialize()
{
  // create system-default overlay scene.
  Scene *s = new OverlayScene();
  overlay_scenes_.push_back(s);
  s->LoadScene();

  // create starting scene.
  switch (GAME->get_boot_mode())
  {
  case GameBootMode::kBootNormal:
  case GameBootMode::kBootArcade:
  case GameBootMode::kBootLR2:
  case GameBootMode::kBootRefresh:
    SceneManager::ChangeScene("LoadingScene");
    break;
  case GameBootMode::kBootTest:
    SceneManager::ChangeScene("TestScene");
    break;
  case GameBootMode::kBootPlay:
    SceneManager::ChangeScene("PlayScene");
    break;
  default:
    R_ASSERT(0);
  }
}

void SceneManager::Cleanup()
{
  if (current_scene_)
  {
    delete current_scene_;
    current_scene_ = 0;
  }
  for (auto *s : overlay_scenes_)
    delete s;
  overlay_scenes_.clear();
}

void SceneManager::Update()
{
  // check is it necessary to change scene
  // StartScene is called here. (time critical process)
  if (next_scene_)
  {
    delete current_scene_;
    current_scene_ = next_scene_;
    next_scene_ = nullptr;

    current_scene_->StartScene();

    // Need to refresh game timer & clear out delta
    // to set exact scene start timing,
    // as much time passed due to scene loading.
    Timer::Update();
    Timer::SystemTimer().ClearDelta();
  }

  float delta = static_cast<float>(Timer::SystemTimer().GetDeltaTime() * 1000);

  // update foreground / background scene first.
  // these scenes should be always updated regardless of dialogs
  for (auto* s : overlay_scenes_)
    s->Update(delta);
  if (background_scene_)
    background_scene_->Update(delta);

  // update main scene
  try
  {
    if (current_scene_ && !GAME->IsPaused())
      current_scene_->Update(delta);
  }
  catch (const RetryException& e)
  {
    /* TODO: create retryexception dialog */
    GAME->AlertMessageBox(e.exception_name(), e.what());
  }
  catch (const RuntimeException& e)
  {
    GAME->AlertMessageBox(e.exception_name(), e.what());
  }
}

void SceneManager::Render()
{
  if (background_scene_)
    background_scene_->Render();

  if (current_scene_)
    current_scene_->Render();

  for (auto* s : overlay_scenes_)
    s->Render();
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


SceneManager& SceneManager::getInstance()
{
  static SceneManager scenemanager_;
  return scenemanager_;
}

void SceneManager::ChangeScene(const std::string &scene_name)
{
  static std::map <std::string, std::function<Scene*()> > sceneCreateFn;
  if (sceneCreateFn.empty())
  {
    sceneCreateFn["TestScene"] = []() { return new TestScene(); };
    sceneCreateFn["LoadingScene"] = []() { return new LoadingScene(); };
    sceneCreateFn["SelectScene"] = []() { return new SelectScene(); };
    sceneCreateFn["DecideScene"] = []() { return new DecideScene(); };
    sceneCreateFn["PlayScene"] = []() { return new PlayScene(); };
    sceneCreateFn["ResultScene"] = []() { return new ResultScene(); };
    //sceneCreateFn["CourseResultScene"] = []() { return new CourseResultScene(); };
  }

  auto &inst = getInstance();
  bool is_exit = false;

  if (inst.next_scene_)
  {
    std::cout << "Warning: Next scene is already set & cached." << std::endl;
    return;
  }

  if (scene_name.empty() || stricmp(scene_name.c_str(), "exit") == 0)
    is_exit = true;

  auto it = sceneCreateFn.find(scene_name);
  if (it == sceneCreateFn.end())
    is_exit = true;

  if (is_exit)
  {
    // prepare to exit game
    Game::Exit();
    return;
  }

  inst.next_scene_ = it->second();

  // LoadScene is done here
  // (time-consuming loading is done which is not time critical
  //  e.g. Texture loading)
  inst.next_scene_->LoadScene();
}

}
