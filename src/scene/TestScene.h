#pragma once
#include "Scene.h"
#include "Sprite.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual void LoadScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);
private:
  Sprite spr_;
};

}