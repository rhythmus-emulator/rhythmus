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
  static void Execute(const std::string &path);

  Script();
  ~Script();

  static Script &getInstance();
};

}