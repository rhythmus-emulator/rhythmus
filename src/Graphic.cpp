#include "Graphic.h"
#include "Game.h"
#include "SceneManager.h"
#include <iostream>

namespace rhythmus
{

static void error_callback(int error, const char* description)
{
  std::cerr << "Error: " << description << std::endl;
}


bool Graphic::Initialize()
{
  Game &game = Game::getInstance();

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
  {
    std::cerr << "Failed to init GLFW." << std::endl;
    exit(EXIT_FAILURE);
  }

  window = glfwCreateWindow(
    game.get_window_width(),
    game.get_window_height(),
    game.get_window_title().c_str(), NULL, NULL
  );
  if (!window)
  {
    std::cerr << "Failed to create window." << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

}

void Graphic::LoopRendering()
{
  int fwidth, fheight;
  while (!glfwWindowShouldClose(window))
  {
    glfwGetFramebufferSize(window, &fwidth, &fheight);
    glViewport(0, 0, fwidth, fheight);
    glClear(GL_COLOR_BUFFER_BIT);

    SceneManager::getInstance().Render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void Graphic::Cleanup()
{
  glfwDestroyWindow(window);
  glfwTerminate();
}

Graphic& Graphic::getInstance()
{
  static class Graphic g;
  return g;
}

Graphic::Graphic()
{
}

Graphic::~Graphic()
{
}

}