#define GLEW_STATIC
#include <GL/glew.h>
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


void Graphic::Initialize()
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

  glfwMakeContextCurrent(window);
  if (glewInit() != GLEW_OK)
  {
    std::cerr << "Failed glewInit()." << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
}

void Graphic::LoopRendering()
{
  int fw, fh;
  while (!glfwWindowShouldClose(window))
  {
    glfwGetFramebufferSize(window, &fw, &fh);
    glViewport(0, 0, fw, fh);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fw, fh, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    SceneManager::getInstance().Render();
    glFlush();

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