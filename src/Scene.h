#pragma once

#include "Game.h"
#include "Timer.h"
#include "BaseObject.h"
#include "ResourceManager.h"
#include "Event.h"
#include <string>
#include <vector>

namespace rhythmus
{

// Parameters with scene theme (read-only).
// All time parameters are in miliseconds.
// @depreciated removed later.
struct ThemeParameter
{
  ThemeParameter();

  std::string gamemode;
  std::string title;
  std::string maker;
  std::string preview;

  // texture transparent color key in RGB. default is 0,0,0
  unsigned char transcolor[3];

  // input starting time of current scene begin
  int begin_input_time;

  // fade in / out time
  int fade_in_time, fade_out_time;

  // time to move next scene (in milisecond)
  // 0 : don't move scene to next scene automatically
  int next_scene_time;

  // other untranslated attributes (may be used later)
  std::map<std::string, std::string> attributes;
};

class SceneTask
{
public:
  SceneTask(const std::string& name, std::function<void()> func);
  void wait_for(float wait_time);
  void wait_cond(std::function<bool()> cond);
  void run();
  bool update(float delta);

  /* @brief update time for running task.
   * @return true if task run (or ran).
   * @depreciate task may modify TaskQueue, so use it with care. */
  bool update_for_run(float delta);
private:
  std::string name_;
  std::function<void()> func_;
  std::function<bool()> cond_;
  float wait_time_;
  bool is_ran_;
};

class SceneTaskQueue
{
public:
  SceneTaskQueue();
  void Enqueue(SceneTask *task);
  void EnqueueFront(SceneTask *task);
  void IsEnqueueable(bool allow_enqueue);
  void FlushAll();
  void DeleteAll();
  void Update(float delta);
private:
  std::list<SceneTask*> tasks_;
  bool allow_enqueue_;
};

/* @brief A screen which is renderable */
class Scene : public BaseObject
{
public:
  Scene();
  virtual ~Scene() { };

  /* @brief load scene resource and prepare to start */
  virtual void LoadScene();

  /* @brief start scene e.g. start scene timer */
  virtual void StartScene();

  /* @brief triggered when scene is finished
   * @warn it does not mean changing scene instantly,
   * which triggers fadeout/transition. */
  void TriggerFadeOut();
  void TriggerFadeIn();

  /* @brief close scene completely & do some misc (e.g. save setting) */
  virtual void CloseScene();

  void EnableInput(bool enable_input);

  /* @brief Event processing */
  virtual bool ProcessEvent(const EventMessage& e) = 0;

  /* @brief Add images to be updated constantly. e.g. Movie */
  void RegisterImage(ImageAuto img);

  ImageAuto GetImageByName(const std::string& name);

  FontAuto GetFontByName(const std::string& name);

  /* @brief load metrics for children. */
  void LoadObjectMetrics(const ThemeMetrics &metrics);

  const ThemeParameter& get_theme_parameter() const;

  /* @brief load metrics to existing object,
   * or create object by given metrics.*/

protected:
  // Theme parameter list (read-only)
  ThemeParameter theme_param_;

  // Theme options
  Setting setting_;

  // image resources loaded by this scene
  std::vector<ImageAuto> images_;

  // font resource loaded by this scene
  std::vector<FontAuto> fonts_;

  // generally used scene queued tasks
  SceneTaskQueue scenetask_;

  // is event triggered at valid time, so it need to be processed?
  // (e.g. input event during scene loading or #IGNOREINPUT --> ignore)
  bool is_input_available() const;
  virtual void doUpdate(float delta);
  virtual void doRenderAfter();

  // next scene mode
  GameSceneMode next_scene_mode_;

private:
  void LoadOptions();
  void SaveOptions();

  /* @brief load scene specific resource. */
  void LoadResource();

  /* @brief load scene specific script 
   * to create and initialize object, event, etc. */
  void LoadScript();

  // fade in/out specified time
  // fade_duration with positive: fade-in
  // fade_duration with negative: fade-out
  float fade_time_, fade_duration_;

  bool is_input_available_;

  // @brief enable sorting objects in LoadScene()
  bool do_sort_objects_;

  // @brief enable scene resource caching for fast reload for this scene.
  bool enable_caching_;

  // currently focused object (if exists)
  BaseObject* focused_object_;

  std::string prev_scene, next_scene;
};

}