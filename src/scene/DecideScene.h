#pragma once

#include "Scene.h"

namespace rhythmus
{

class DecideScene : public Scene
{
public:
  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);

  virtual const std::string GetSceneName() const;
};

}