#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h> /* TODO: move it to Error.h */
#define ASSERT(x) assert(x)
#define ASSERT_GL() ASSERT(glGetError() == 0)
#define ASSERT_GL_VAL(x) ASSERT((x = glGetError()) == 0)

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