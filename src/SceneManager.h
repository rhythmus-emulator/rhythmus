#pragma once
#include "Scene.h"
#include "Game.h"

namespace rhythmus
{

class SceneManager
{
public:
  void Initialize();
  void Render();
  void SendEvent(const GameEvent& e);
  void ChangeScene(bool force=false);
  Scene* get_current_scene();

  Timer& GetSceneTimer();

  static SceneManager& getInstance();
private:
  SceneManager();
  ~SceneManager();

  // currently displaying scene
  Scene* current_scene_;

  // Create next scene using current gamestate.
  Scene* CreateNextScene();

  // Scene timer
  Timer timer_scene_;

  // Scene fadeout timer
  Timer timer_fadeout_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

