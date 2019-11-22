#pragma once

#include "Scene.h"

namespace rhythmus
{

class ResultScene : public Scene
{
public:
  ResultScene();
  virtual void ProcessInputEvent(const InputEvent& e);
};

}