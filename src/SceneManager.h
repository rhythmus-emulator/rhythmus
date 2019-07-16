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

  Scene* scene_current_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

