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

private:
  virtual void doUpdate(float delta);

  /**
   * 0: Waiting for playing (e.g. loading)
   * 1: Playing started
   * 2: Playing paused
   * 3: Playing stopped (cannot return to 1)
   */
  int play_status_;
};

}