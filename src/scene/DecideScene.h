#pragma once

#include "Scene.h"

namespace rhythmus
{

class DecideScene : public Scene
{
public:
  DecideScene();
  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);
};

}