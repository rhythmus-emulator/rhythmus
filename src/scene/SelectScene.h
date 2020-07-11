#pragma once

#include "Scene.h"
#include "Sound.h"
#include "object/MusicWheel.h"

namespace rhythmus
{


class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  MusicWheel wheel_;
};

}