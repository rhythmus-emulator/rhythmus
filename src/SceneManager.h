#pragma once
#include "Scene.h"
#include "Game.h"
#include "Event.h"
#include "SceneMetrics.h"

namespace rhythmus
{

class SceneManager : public EventReceiver
{
public:
  void Initialize();
  void Cleanup();
  void Update();
  void Render();
  virtual bool OnEvent(const EventMessage& msg);
  void ChangeScene(bool force=false);

  static Scene* get_current_scene();
  static Timer& GetSceneTimer();
  static uint32_t GetSceneTickTime();
  static SceneManager& getInstance();
  static Setting& getSetting();

  static ThemeMetrics *getMetrics(const std::string &name);
  static ThemeMetrics *createMetrics(const std::string &name);

private:
  SceneManager();
  ~SceneManager();

  // Scene preferences
  Setting setting_;

  // currently displaying scene
  Scene* current_scene_;

  // Create next scene using current gamestate.
  Scene* CreateNextScene();

  // Scene timer
  Timer timer_scene_;

  // Theme metrics
  ThemeMetricsList metrics_list_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

