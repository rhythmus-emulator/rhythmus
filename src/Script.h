#pragma once

#include <string>

namespace rhythmus
{

class Scene;

/**
 * @brief
 * Processes all types scripts to modify current gamestate, scene object, etc.
 * Different from Metric/Settings, as it doesn't save/load it's own state.
 */
class Script
{
public:
  static void ExecuteFromPath(const std::string &path);

  Script();
  ~Script();

  void SetContextScene(Scene *scene);
  void SetFlag(int flag_no, int value);
  int GetFlag(int flag_no) const;
  void SetString(size_t idx, const std::string &value);
  const std::string& GetString(size_t idx) const;
  void SetNumber(size_t idx, int number);
  int GetNumber(size_t idx) const;
  void SetButtonNumber(size_t idx, int number);
  void SetSliderNumber(size_t idx, int number);

  static Script &getInstance();

private:
  void ExecuteLR2Script(const std::string &path);
  
  /* for LR2 op code */
  int flags_[1000];

  /**
   * @brief for number cache (LR2)
   * @detail
   * 0~1000 : normal use of number cache
   * 1000~1500 : button value cache
   * 1500~1600 : slider value cache (0 ~ 100)
   * 1600~1700 : bargraph value cache (0 ~ 100)
   */
  int numbers_[3000];

  /* for string cache (LR2) */
  std::string strings_[1000];

  /* script context scene */
  Scene *scene_;
};

}