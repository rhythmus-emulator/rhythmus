#pragma once

#include "Scene.h"

namespace rhythmus
{

class DecideScene : public Scene
{
public:
  DecideScene();
  virtual void LoadScene();
  virtual bool ProcessEvent(const EventMessage& e);
};

}