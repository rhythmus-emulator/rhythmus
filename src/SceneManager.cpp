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
#include "Script.h"
#include "Logger.h"
#include "common.h"

namespace rhythmus
{

// ------------------------- class SceneManager

SceneManager::SceneManager()
  : current_scene_(nullptr), background_scene_(nullptr), next_scene_(nullptr),
    hovered_obj_(nullptr), focused_obj_(nullptr), dragging_obj_(nullptr),
    px(.0f), py(.0f)
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

  // check and set scene theme
  std::vector<std::string> theme_list;
  SCENEMAN->GetThemeList(theme_list);
  PrefOptionList theme_pref("theme", theme_list);
  SCENEMAN->SetTheme(theme_pref.get());

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
  if (next_scene_) {
    loading_next_scene = true;
    if (!next_scene_->IsLoading()) {
      delete current_scene_;
      current_scene_ = next_scene_;
      next_scene_ = nullptr;

      current_scene_->OnReady();
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
  try {
    if (!loading_next_scene && current_scene_ && !GAME->IsPaused())
      current_scene_->Update(delta);
  }
  catch (const RetryException& e) {
    /* TODO: create retryexception dialog */
    GAME->AlertMessageBox(e.exception_name(), e.what());
  }
  catch (const RuntimeException& e) {
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

  // if clicked for debug purpose,
  // then search for any object collided to cursor.
  if (EVENTMAN->GetStatus(RI_KEY_LEFT_CONTROL) && EVENTMAN->GetStatus(RI_KEY_LEFT_ALT) &&
      e.type() == InputEvents::kOnCursorDown && current_scene_)
  {
    curr_hover_object = current_scene_->GetChildAtPosition(x, y);
    if (e.GetButton() == 0)
    {
      if (curr_hover_object)
      {
        GAME->SetClipBoard(curr_hover_object->toString());
        //GAME->PopupMessage("Debug Info Copied.");
      }
    }
    else {
      current_scene_->RemoveChild(curr_hover_object);
    }
    return;
  }

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

    if (hovered_obj_ != curr_hover_object)
    {
      if (hovered_obj_) hovered_obj_->SetHovered(false);
      if (curr_hover_object) curr_hover_object->SetHovered(true);
      hovered_obj_ = curr_hover_object;
    }
  }

  // If searched object is go-through object (propagate click event),
  // then search for clickable object again.
  // TODO

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
    {
      dragging_obj_->OnDrag(x - px, y - py);
      dragging_obj_ = nullptr;
    }
    if (hovered_obj_)
      hovered_obj_->Click();
  }
  else if (e.type() == InputEvents::kOnText && focused_obj_)
  {
    focused_obj_->OnText(e.Codepoint());
  }

  px = x;
  py = y;

  //
  // propagate event to scene
  //
  if (current_scene_ && current_scene_->IsInputAvailable())
    current_scene_->ProcessInputEvent(e);
}

void SceneManager::GetThemeList(std::vector<std::string>& list)
{
  const std::string _theme_directory = "themes/";
  std::vector<std::string> paths;
  list.clear();
  PATH->GetDescendantDirectories(_theme_directory, paths);
  for (const auto& s : paths) {
    std::string theme_name = s.substr(_theme_directory.size());
    if (theme_name == "_fallback") continue;
    list.push_back(theme_name);
  }
}

enum ThemeType
{
  TT_None,
  TT_LR2,
  TT_Stepmania
};

void SceneManager::SetTheme(const std::string& name)
{
  ThemeType theme_type = TT_None;
  const std::string theme_path = format_string("themes/%s/*", name.c_str());
  const std::string theme_LR2_path = format_string("themes/%s/select", name.c_str());
  std::vector<std::string> files;
  scene_scripts_.clear();

  // detect theme type by checking files
  PATH->GetAllPaths(theme_path, files);
  if (std::find(files.begin(), files.end(), theme_LR2_path)
      != files.end()) {
    theme_type = TT_LR2;
  }
  files.clear();

  switch (theme_type) {
  case TT_LR2:
    // fill scene_scripts_ by executing all scripts
    // (don't execute script, only running as pre-loading mode for loading configs)
    Script::SetPreloadMode(true);
    PATH->GetAllPaths(format_string("themes/%s/*/*.lr2skin", name.c_str()), files);
    for (const auto& f : files) {
      Script::Load(f, nullptr);
    }
    if (scene_scripts_.size() == 0) {
      Logger::Warn("No scene scripts detected, maybe invalid LR2SKIN file?");
    }
    Script::SetPreloadMode(false);
    break;
  default:
    R_ASSERT(0 && "Unknown Theme Type");
    break;
  }
}

void SceneManager::SetSceneScript(const std::string& name, const std::string& script_path)
{
  scene_scripts_[name] = script_path;
}

std::string SceneManager::GetSceneScript(Scene *s)
{
  if (!s || s->get_name().empty()) return "";
  auto i = scene_scripts_.find(s->get_name());
  if (i != scene_scripts_.end())
    return i->second;
  return "";
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
