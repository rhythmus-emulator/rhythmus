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

  // input prohibit time when countdown is done
  int end_input_time;

  // fade in / out time
  int fade_in_time, fade_out_time;

  // scene maximum / minimum time to be shown.
  // @warn  might not work in some type of scene.
  int min_time, max_time;
};

// User-customizable theme option
struct ThemeOption
{
  // option id (MUST required)
  std::string id;

  // option name (same to ID if not set)
  std::string name;

  // option description (nullable)
  std::string desc;

  // theme option type. hide is 0, option is 1, file is 2.
  int type;

  // selectable options. separated in comma, or file filter.
  std::string options;

  // selected value.
  std::string selected;

  /* @brief Get selection list from current ThemeOption */
  void GetSelectionList(std::vector<std::string> &list);

  /* @brief Get current selected value */
  std::string GetSelectedValue();
};

/* @brief A screen which is renderable */
class Scene : public BaseObject
{
public:
  virtual ~Scene() { };

  /* @brief load scene resource and prepare to start */
  virtual void LoadScene();

  /* @brief start scene e.g. start scene timer */
  virtual void StartScene() = 0;

  /* @brief prepare to finish scene e.g. prepare next scene */
  virtual void CloseScene();

  /* @brief Process event in this scene */
  virtual bool ProcessEvent(const EventMessage& e) = 0;

  /* @brief Add images to be updated constantly. e.g. Movie */
  void RegisterImage(ImageAuto img);

  ImageAuto GetImageByName(const std::string& name);

  FontAuto GetFontByName(const std::string& name);

  virtual void LoadProperty(const std::string& prop_name, const std::string& value);

protected:
  // User-customizable theme option
  ThemeParameter theme_param_;

  // User customizable theme option list.
  std::vector<ThemeOption> theme_options_;

  // image resources loaded by this scene
  std::vector<ImageAuto> images_;

  // font resource loaded by this scene
  std::vector<FontAuto> fonts_;

private:
  virtual void doUpdate(float delta);

  void LoadOptions();
  void SaveOptions();

  void LoadFromCsv(const std::string& filepath);
  void SetThemeConfig(const std::string& key, const std::string& value);
  ThemeOption* GetThemeOption(const std::string& key);

  // Generate select list from theme option by second parameter
  // return select list count.
  static void GetThemeOptionSelectList(
    const ThemeOption &option, std::vector<std::string>& selectable);
};

}