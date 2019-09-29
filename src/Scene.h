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
struct ThemeParameter
{
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
  virtual void FinishScene();

  /* @brief close scene completely & do some misc (e.g. save setting) */
  virtual void CloseScene();

  /* @brief Event processing */
  virtual bool ProcessEvent(const EventMessage& e) = 0;

  /* @brief Add images to be updated constantly. e.g. Movie */
  void RegisterImage(ImageAuto img);

  ImageAuto GetImageByName(const std::string& name);

  FontAuto GetFontByName(const std::string& name);

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

  void TriggerFadeIn(float duration);
  void TriggerFadeOut(float duration);
  void QueueSceneEvent(float delta, int event_id);

  const ThemeParameter& get_theme_parameter() const;

protected:
  // Theme parameter list (read-only)
  ThemeParameter theme_param_;

  // Theme options
  Setting setting_;

  // image resources loaded by this scene
  std::vector<ImageAuto> images_;

  // font resource loaded by this scene
  std::vector<FontAuto> fonts_;

  QueuedEventCache eventqueue_;

  // is event triggered at valid time, so it need to be processed?
  // (e.g. input event during scene loading or #IGNOREINPUT --> ignore)
  bool IsEventValidTime(const EventMessage& e) const;
  virtual void doUpdate(float delta);
  virtual void doRenderAfter();

  // next scene mode
  GameSceneMode next_scene_mode_;

private:
  void LoadOptions();
  void SaveOptions();
  void LoadFromCsv(const std::string& filepath);

  // fade in/out specified time
  // fade_duration with positive: fade-in
  // fade_duration with negative: fade-out
  float fade_time_, fade_duration_;

  // input available gametime
  uint32_t input_available_time_;

  // currently focused object (if exists)
  BaseObject* focused_object_;
};

}