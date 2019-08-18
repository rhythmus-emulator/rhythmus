#pragma once

#include "Scene.h"

namespace rhythmus
{

class SelectScene : public Scene
{
  virtual void StartScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);

  virtual const std::string GetSceneName() const;
};

}