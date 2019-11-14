#pragma once
#include "Scene.h"
#include "Game.h"
#include "Event.h"
#include "SceneMetrics.h"

namespace rhythmus
{

class SceneManager : public InputEventReceiver
{
public:
  void Initialize();
  void Cleanup();
  void Update();
  void Render();

  static Scene* get_current_scene();
  static Timer& GetSceneTimer();
  static uint32_t GetSceneTickTime();
  static SceneManager& getInstance();
  static Setting& getSetting();

  static ThemeMetrics *getMetrics(const std::string &name);
  static ThemeMetrics *createMetrics(const std::string &name);
  static int getVisible(size_t index);
  static void setVisible(size_t index, int value);

  static void ChangeScene(const std::string &scene_name);

  /* from InputEventReceiver */
  virtual void OnInputEvent(const InputEvent& e);

private:
  SceneManager();
  ~SceneManager();

  // Scene preferences
  Setting setting_;

  // currently displaying scene
  Scene *current_scene_;

  // next scene cached
  Scene *next_scene_;

  // Scene timer
  Timer timer_scene_;

  // Theme metrics
  ThemeMetricsList metrics_list_;

  // group data of visibility (up to 1000 groups)
  int visible_groups_[1000];
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

