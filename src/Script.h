#pragma once

#include <string>

namespace rhythmus
{

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
  static Script &getInstance();

private:
  void ExecuteLR2Script(const std::string &path);
  
  /* for LR2 op code */
  int flags_[1000];
};

}