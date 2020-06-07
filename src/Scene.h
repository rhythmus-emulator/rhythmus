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

class SceneTask
{
public:
  SceneTask(const std::string& name, std::function<void()> func);
  void wait_for(float wait_time);
  void wait_for(int wait_time);
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

  virtual void Load(const MetricGroup& m);

  /* @brief triggered when scene is finished
   * @warn it does not mean changing scene instantly,
   * which triggers fadeout/transition. */
  void FadeOutScene(bool next);
  void TriggerFadeIn();

  /* @brief close scene completely & do some misc (e.g. save setting) */
  virtual void CloseScene(bool next);

  void EnableInput(bool enable_input);
  bool IsInputAvailable() const;

  /* @brief Event processing for scene specific action */
  virtual void ProcessInputEvent(const InputEvent& e);

protected:
  // generally used scene queued tasks
  SceneTaskQueue scenetask_;

  // is event triggered at valid time, so it need to be processed?
  // (e.g. input event during scene loading or #IGNOREINPUT --> ignore)
  virtual void doUpdate(double delta);
  virtual void doRenderAfter();

  std::string prev_scene_, next_scene_;

private:
  // fade in/out specified time
  // fade_duration with positive: fade-in
  // fade_duration with negative: fade-out
  double fade_time_, fade_duration_;

  // fade in / out time
  int fade_in_time_, fade_out_time_;

  // input availablity
  bool is_input_available_;

  // input starting time of current scene begin
  int begin_input_time_;

  // time to move next scene (in milisecond)
  // 0 : don't move scene to next scene automatically
  int next_scene_time_;

  // @brief enable sorting objects in LoadScene()
  bool do_sort_objects_;

  // @brief enable scene resource caching for fast reload for this scene.
  bool enable_caching_;

  // theme-specific metric data used for loading scene.
  MetricGroup metric_;

  // currently focused object (if exists)
  BaseObject* focused_object_;
};

}
