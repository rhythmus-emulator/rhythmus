#pragma once

#include "Game.h"
#include "ResourceManager.h"
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
};

class Scene;

/* @brief util to help scene loading */
class SceneLoader
{
public:
  SceneLoader(Scene *s);
  virtual ~SceneLoader();

  virtual void Load(const std::string& filepath) = 0;

  void GetThemeOptions(std::vector<ThemeOption>& theme_options);
  void GetSpriteObjects(std::vector<SpriteAuto>& sprites);
  void GetImages(std::vector<ImageAuto>& images);
  const ThemeParameter& GetThemeParameter();

protected:
  Scene *scene_;

  ThemeParameter theme_param_;
  std::vector<ImageAuto> images_;
  std::vector<ThemeOption> theme_options_;
  std::vector<SpriteAuto> sprites_;
};

/* @brief A screen which is renderable */
class Scene
{
public:
  virtual ~Scene() { };

  virtual void LoadScene(SceneLoader *scene_loader);
  virtual void CloseScene() = 0;
  virtual void Render() = 0;
  virtual void ProcessEvent(const GameEvent& e) = 0;

  void LoadOptions();
  void SaveOptions();

  virtual const std::string GetSceneName() const = 0;

  bool operator==(const Scene& s) {
    return s.GetSceneName() == GetSceneName();
  }

protected:
  // User-customizable theme option
  ThemeParameter theme_param_;

  // User customizable theme option list.
  std::vector<ThemeOption> theme_options_;

  // image resources loaded by this scene
  std::vector<ImageAuto> images_;

  // font resource loaded by this scene
  std::vector<FontAuto> fonts_;

  // sprites to be rendered
  std::vector<SpriteAuto> sprites_;

protected:
  // Generate select list from theme option by second parameter
  // return select list count.
  static void GetThemeOptionSelectList(
    const ThemeOption &option, std::vector<std::string>& selectable);
};

}