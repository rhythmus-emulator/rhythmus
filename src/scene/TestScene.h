#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "Font.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual ~TestScene();

  virtual void LoadScene();
  virtual void CloseScene();
  virtual void Render();
  virtual void ProcessEvent(const GameEvent& e);
private:
  Sprite spr_;
  Sprite spr2_;
  Font font_;
};

}