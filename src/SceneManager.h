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

  static SceneManager& getInstance();
private:
  SceneManager();
  ~SceneManager();

  Scene* current_scene_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

