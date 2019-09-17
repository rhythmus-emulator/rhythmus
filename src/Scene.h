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

// Parameters with scene theme.
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
};

// User-customizable theme option
struct ThemeOption
{
  // option name
  std::string name;

  // option description (nullable)
  std::string desc;

  // theme option type. (option or file)
  std::string type;

  // selectable options. separated in comma, or file filter.
  std::string options;

  // selected value.
  std::string selected;
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

  /* @brief prepare to finish scene
   * @warn it does not mean changing scene instantly,
   * which triggers fadeout/transition. */
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

protected:
  // User-customizable theme option
  ThemeParameter theme_param_;

  // User customizable theme option list.
  std::vector<ThemeOption> theme_options_;

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

private:

  void LoadOptions();
  void SaveOptions();

  void LoadFromCsv(const std::string& filepath);
  void SetThemeConfig(const std::string& key, const std::string& value);
  ThemeOption* GetThemeOption(const std::string& key);

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