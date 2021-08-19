#pragma once

#include "Scene.h"
#include "Sound.h"

namespace rhythmus
{

class Wheel;
class LR2MusicWheel;

class SelectScene : public Scene
{
public:
  SelectScene();

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);
  using Scene::RunCommandLR2;
  virtual void RunCommandLR2(const std::string& command, const LR2FnArgs& args);

  Wheel* GetWheelObject();
  LR2MusicWheel* GetLR2WheelObject();

private:
  Wheel* wheel_;
  Sound bgm_;
};

}