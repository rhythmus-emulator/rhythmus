#pragma once
#include <GLFW/glfw3.h>

namespace rhythmus
{

/**
 * @brief
 * Contains graphic context of game.
 * Singleton class.
 */

class Graphic
{
public:
  void Initialize();
  void Cleanup();
  void LoopRendering();

  static Graphic& getInstance();
private:
  Graphic();
  ~Graphic();

  GLFWwindow* window;
};

}