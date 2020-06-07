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
  : current_scene_(nullptr), background_scene_(nullptr), next_scene_(nullptr),
    hovered_obj_(nullptr), focused_obj_(nullptr), dragging_obj_(nullptr)
{
}

SceneManager::~SceneManager()
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

void SceneManager::Initialize()
{
  // create object.
  // The object will be automatically registered to InputEventManager,
  // so be careful to call this method after initializing Event module.
  R_ASSERT(SCENEMAN == nullptr);
  SCENEMAN = new SceneManager();

  // create system-default overlay scene.
  Scene *s = new OverlayScene();
  SCENEMAN->overlay_scenes_.push_back(s);
  s->LoadScene();

  // create starting scene.
  switch (GAME->get_boot_mode())
  {
  case GameBootMode::kBootNormal:
  case GameBootMode::kBootArcade:
  case GameBootMode::kBootLR2:
  case GameBootMode::kBootRefresh:
    SCENEMAN->ChangeScene("LoadingScene");
    break;
  case GameBootMode::kBootTest:
    SCENEMAN->ChangeScene("TestScene");
    break;
  case GameBootMode::kBootPlay:
    SCENEMAN->ChangeScene("PlayScene");
    break;
  default:
    R_ASSERT(0);
  }
}

void SceneManager::Cleanup()
{
  delete SCENEMAN;
  SCENEMAN = nullptr;
}

void SceneManager::Update()
{
  bool loading_next_scene = false;

  // Check is it okay to change scene (when loading is done)
  // StartScene is called here. (time critical process)
  // XXX: Checking scene loading by ResourceManager is too naive...
  if (next_scene_)
  {
    loading_next_scene = true;
    if (!ResourceManager::IsLoading())
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

      loading_next_scene = false;
    }
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
    if (!loading_next_scene && current_scene_ && !GAME->IsPaused())
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

void SceneManager::ClearFocus()
{
  hovered_obj_ = nullptr;
  focused_obj_ = nullptr;
}

void SceneManager::ClearFocus(BaseObject *obj)
{
  if (hovered_obj_ == obj) hovered_obj_ = nullptr;
  if (focused_obj_ == obj) focused_obj_ = nullptr;
}

BaseObject *SceneManager::GetHoveredObject() { return hovered_obj_; }
BaseObject *SceneManager::GetFocusedObject() { return focused_obj_; }
BaseObject *SceneManager::GetDraggingObject() { return dragging_obj_; }

void SceneManager::OnInputEvent(const InputEvent& e)
{
  //
  // Check out focusing of objects
  // Check hovering on object at the same time.
  //
  float x = (float)e.GetX();
  float y = (float)e.GetY();
  BaseObject *curr_hover_object = nullptr;

  // XXX: on touch event on mobile?
  if (e.type() == InputEvents::kOnCursorMove
   || e.type() == InputEvents::kOnCursorDown)
  {
    for (auto *os : overlay_scenes_)
    {
      if (curr_hover_object) break;
      if (!os->IsInputAvailable()) continue;
      curr_hover_object = os->GetChildAtPosition(x, y);
    }
    if (!curr_hover_object && current_scene_ && current_scene_->IsInputAvailable())
      curr_hover_object = current_scene_->GetChildAtPosition(x, y);
  }
  if (hovered_obj_ != curr_hover_object)
  {
    if (hovered_obj_) hovered_obj_->SetHovered(false);
    if (curr_hover_object) curr_hover_object->SetHovered(true);
    hovered_obj_ = curr_hover_object;
  }

  // If object is focusable, then it get focus status.
  // If object is draggable, it is only focused while dragging.
  if (e.type() == InputEvents::kOnCursorDown && focused_obj_ != hovered_obj_)
  {
    if (focused_obj_)
    {
      focused_obj_->SetFocused(false);
      focused_obj_ = nullptr;
    }

    if (hovered_obj_)
    {
      if (hovered_obj_->IsFocusable())
      {
        focused_obj_ = hovered_obj_;
        focused_obj_->SetFocused(true);
      }
      if (hovered_obj_->IsDraggable())
      {
        dragging_obj_ = hovered_obj_;
      }
    }
  }
  else if (e.type() == InputEvents::kOnCursorMove)
  {
    if (dragging_obj_)
    {
      dragging_obj_->OnDrag(x - px, y - py);
    }
  }
  else if (e.type() == InputEvents::kOnCursorClick /* Up */)
  {
    if (dragging_obj_)
      dragging_obj_ = nullptr;
    if (hovered_obj_)
      hovered_obj_->Click();
  }
  else if (e.type() == InputEvents::kOnText && focused_obj_)
  {
    // TODO: SetText()
  }

  px = x;
  py = y;

  //
  // propagate event to scene
  //
  if (current_scene_ && current_scene_->IsInputAvailable())
    current_scene_->ProcessInputEvent(e);
}

Scene* SceneManager::get_current_scene()
{
  return current_scene_;
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

  bool is_exit = false;

  if (next_scene_)
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

  next_scene_ = it->second();

  // LoadScene is done here
  // @warn
  // This process might be async.
  // StartScene() will be called after LoadScene is complete.
  next_scene_->LoadScene();
}

SceneManager *SCENEMAN = nullptr;

}
