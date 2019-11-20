#pragma once
#include "Scene.h"
#include "Sprite.h"
#include "Font.h"
#include "LR2/LR2Font.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  TestScene();
  virtual ~TestScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene();
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  Sprite spr_;
  Sprite spr2_;
  Sprite spr_bg_;
  Text text_;
  Text lr2text_;
};

}