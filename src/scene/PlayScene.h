#pragma once

#include "Scene.h"

namespace rhythmus
{

class PlayScene : public Scene
{
public:
  PlayScene();
  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual bool ProcessEvent(const EventMessage& e);
};

}