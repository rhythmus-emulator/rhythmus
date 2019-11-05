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
  void LoadProperty(const std::string& name, const std::string& value);

  static Scene* get_current_scene();
  static Timer& GetSceneTimer();
  static uint32_t GetSceneTickTime();
  static SceneManager& getInstance();
  static Setting& getSetting();

  static ThemeMetrics *getMetrics(const std::string &name);
  static void createMetrics(const std::string &name);
  static ThemeMetricsList &getMetricList();

  /**
   * @brief
   * bgm : always loops
   * others : not looped
   */
  struct Soundset
  {
    std::string bgm_startup;
    std::string bgm_title;
    std::string bgm_option;
    std::string bgm_keyconfig;
    std::string bgm_free;
    std::string bgm_standard;
    std::string bgm_extra;
    std::string bgm_profile;
    std::string bgm_staffroll;
    std::string bgm_result_clear;
    std::string bgm_result_failed;
    std::string bgm_courseresult_clear;
    std::string bgm_courseresult_failed;
    std::string scroll;
    std::string folder_open;
    std::string folder_close;
    std::string menu_open;
    std::string menu_close;
    std::string difficulty_change;
    std::string change;
    std::string cancel;
    std::string decide;
    std::string tap;
    std::string play_loading;
    std::string play_start;
    std::string play_abort;
    std::string play_clear;
    std::string play_failed;
    std::string play_fullcombo;
    std::string result;
    std::string result_failed;
    std::string result_clear;
    std::string result_fullcombo;
    std::string mine;
    std::string screenshot;
  };
  void LoadSoundset();
  static const Soundset& getSoundset();

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

  // Soundset
  Soundset soundset_;

  // Theme metrics
  ThemeMetricsList metrics_list_;
};

/* singleton object. */
extern class SceneManager gSceneManager;

}

