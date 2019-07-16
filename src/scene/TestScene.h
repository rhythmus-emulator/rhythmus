#pragma once
#include "Scene.h"
#include "../Image.h"

namespace rhythmus
{

class TestScene : public Scene
{
public:
  virtual void LoadScene();
  virtual void CloseScene();
  virtual void Render();
private:
  ImageAuto img_;
};

}