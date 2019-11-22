#pragma once

#include "Scene.h"

namespace rhythmus
{

class DecideScene : public Scene
{
public:
  DecideScene();
  virtual void ProcessInputEvent(const InputEvent& e);
};

}