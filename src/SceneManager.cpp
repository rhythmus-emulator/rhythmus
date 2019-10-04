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
  if (!setting_.Open("../config/scene.xml"))
  {
    std::cerr << "Cannot open Scene preference file. use default value." << std::endl;
  }

  // load soundset.
  LoadSoundset();

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

void SceneManager::LoadProperty(const std::string& name, const std::string& value)
{
  // Soundset
  if (name == "#SELECT_SOUND")
  {
    soundset_.bgm_free = value;
    soundset_.bgm_standard = value;
  }
  else if (name == "#DECIDE_SOUND")
  {
    soundset_.decide = value;
  }
  else if (name == "#EXSELECT_SOUND")
  {
    soundset_.bgm_extra = value;
  }
  else if (name == "#EXDECIDE_SOUND")
  {
    // TODO?
  }
  else if (name == "#FOLDER_OPEN_SOUND")
  {
    soundset_.folder_open = value;
  }
  else if (name == "#FOLDER_CLOSE_SOUND")
  {
    soundset_.folder_close = value;
  }
  else if (name == "#PANEL_OPEN_SOUND")
  {
    soundset_.menu_open = value;
  }
  else if (name == "#PANEL_CLOSE_SOUND")
  {
    soundset_.menu_close = value;
  }
  else if (name == "#OPTION_CHANGE_SOUND")
  {
    soundset_.change = value;
  }
  else if (name == "#DIFFICULTY_SOUND")
  {
    soundset_.difficulty_change = value;
  }
  else if (name == "#SCREENSHOT_SOUND")
  {
    soundset_.screenshot = value;
  }
  else if (name == "#CLEAR_SOUND")
  {
    soundset_.bgm_result_clear = value;
  }
  else if (name == "#FAIL_SOUND")
  {
    soundset_.bgm_result_failed = value;
  }
  else if (name == "#COURSECLEAR_SOUND")
  {
    soundset_.bgm_courseresult_clear = value;
  }
  else if (name == "#COURSEFAIL_SOUND")
  {
    soundset_.bgm_courseresult_failed = value;
  }
  else if (name == "#STOP_SOUND")
  {
    soundset_.play_abort = value;
    soundset_.play_failed = value;
  }
  else if (name == "#MINE_SOUND")
  {
    soundset_.mine = value;
  }
  else if (name == "#SCRATCH_SOUND")
  {
    soundset_.scroll = value;
  }
  else if (name == "#TAP_SOUND")
  {
    // not original feature
    soundset_.tap = value;
  }
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

void SceneManager::LoadSoundset()
{
  // TODO: load soundset file path setting from game
  std::string soundset_path = "../sound/lr2.lr2ss";

  auto &sm = getInstance();
  std::string ext = GetExtension(soundset_path);
  if (ext == "lr2ss")
  {
    LR2SceneLoader loader;
    loader.SetSubStitutePath("LR2files", "..");
    loader.Load(soundset_path);
    for (auto &v : loader)
    {
      std::string vv = Substitute(
        GetFirstParam(v.second), "LR2files", ".."
      );
      sm.LoadProperty(v.first + "_SOUND", vv);
    }
  }
  else {
    std::cerr << "Soundset file unsupported: " << soundset_path << std::endl;
  }
}

const SceneManager::Soundset& SceneManager::getSoundset()
{
  return SceneManager::getInstance().soundset_;
}

}