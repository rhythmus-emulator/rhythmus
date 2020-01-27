#pragma once
#include "Scene.h"
#include "Game.h"
#include "Event.h"

namespace rhythmus
{

class SceneManager : public InputEventReceiver
{
public:
  void Initialize();
  void Cleanup();
  void Update();
  void Render();

  static Scene* get_current_scene();
  static SceneManager& getInstance();

  static void ChangeScene(const std::string &scene_name);

  /* from InputEventReceiver */
  virtual void OnInputEvent(const InputEvent& e);

private:
  SceneManager();
  ~SceneManager();

  // overlay-displayed scene
  // first element MUST be OverlayScene object.
  std::vector<Scene*> overlay_scenes_;

  // currently displaying scene
  Scene *current_scene_;

  // background scene (mostly for BGA)
  Scene* background_scene_;

  // next scene cached
  Scene *next_scene_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

