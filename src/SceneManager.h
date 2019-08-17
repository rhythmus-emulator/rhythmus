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

  static SceneManager& getInstance();
private:
  SceneManager();
  ~SceneManager();

  // currently displaying scene
  Scene* current_scene_;

  Scene* CreateNextScene();
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

