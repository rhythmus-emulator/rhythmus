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
  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

private:
  virtual void doUpdate(float delta);

  /* task for playscene */
  SceneTaskQueue playscenetask_;

  /**
   * 0: Waiting for playing (e.g. loading)
   * 1: Playing started
   * 2: Playing paused
   * 3: Playing stopped (cannot return to 1)
   */
  int play_status_;

  /* @brief play params for PlayScene */
  struct {
    int playside;
    int load_wait_time;
    int ready_time;
  } theme_play_param_;
};

}