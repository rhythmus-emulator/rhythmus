#pragma once
#include "Scene.h"
#include "Game.h"
#include "Event.h"

namespace rhythmus
{

class BaseObject;

class SceneManager : public InputEventReceiver
{
public:
  static void Initialize();
  static void Cleanup();

  void Update();
  void Render();

  void GetThemeList(std::vector<std::string>& list);
  void LoadThemeConfig(const std::string& name);
  void SetSceneScript(const std::string& name, const std::string& script_path);
  std::string GetSceneScript(Scene *s);

  Scene* get_current_scene();

  void ChangeScene(const std::string &scene_name);

  /* clear out focus */
  void ClearFocus();

  /* clear out focus of specific object
   * This must be called before deleting object.
   * If not, SceneManager may refer to dangling pointer. */
  void ClearFocus(BaseObject *obj);

  BaseObject *GetHoveredObject();
  BaseObject *GetFocusedObject();
  BaseObject *GetDraggingObject();

  /* from InputEventReceiver */
  virtual void OnInputEvent(const InputEvent& e);

private:
  SceneManager();
  ~SceneManager();

  // current theme name
  std::string theme_;

  // theme scripts for specific scene
  std::map<std::string, std::string> scene_scripts_;

  // overlay-displayed scene
  // first element MUST be OverlayScene object.
  std::vector<Scene*> overlay_scenes_;

  // currently displaying scene
  Scene *current_scene_;

  // background scene (mostly for BGA)
  Scene* background_scene_;

  // next scene cached
  Scene *next_scene_;

  // currently hovered object
  BaseObject *hovered_obj_;

  // currently focused object
  BaseObject *focused_obj_;

  // currently dragging object
  BaseObject *dragging_obj_;

  // previous x, y coordinate (for dragging event)
  float px, py;
};

/* singleton object. */
extern SceneManager *SCENEMAN;

}

