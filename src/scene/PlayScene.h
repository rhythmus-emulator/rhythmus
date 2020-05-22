#pragma once

#include "Scene.h"
#include "object/NoteField.h"

namespace rhythmus
{

class PlayScene : public Scene
{
public:
  PlayScene();

  virtual void Load(const MetricGroup& m);

  virtual void LoadScene();
  virtual void StartScene();
  virtual void CloseScene(bool next);
  virtual void ProcessInputEvent(const InputEvent& e);

private:
  virtual void doUpdate(double delta);

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

  /* @brief
   * NoteField per player
   * (currently only up to 4 player?) */
  NoteField notefield_[4];
};

}
