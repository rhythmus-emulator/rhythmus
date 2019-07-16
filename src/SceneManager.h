#pragma once
#include "scene/Scene.h"

namespace rhythmus
{

class SceneManager
{
public:
  void Initialize();
  void Render();

  static SceneManager& getInstance();
private:
  SceneManager();
  ~SceneManager();

  Scene* current_scene_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

