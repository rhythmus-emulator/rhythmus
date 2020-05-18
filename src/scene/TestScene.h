#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "object/Text.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual ~TestScene();

  virtual void LoadScene();
  virtual void ProcessInputEvent(const InputEvent& e);
};

}