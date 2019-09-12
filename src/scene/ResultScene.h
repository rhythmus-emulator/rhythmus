#pragma once

#include "Scene.h"

namespace rhythmus
{

class ResultScene : public Scene
{
public:
  ResultScene();
  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);
};

}