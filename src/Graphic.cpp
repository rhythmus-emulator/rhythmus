#include "Graphic.h"
#include "Game.h"
#include "SceneManager.h"
#include <iostream>

namespace rhythmus
{

// An global variable indicating window width / height
int sWidth, sHeight;

static void error_callback(int error, const char* description)
{
  std::cerr << "Error: " << description << std::endl;
}

void on_resize(GLFWwindow* w, GLint width, GLint height)
{
  sWidth = width;
  sHeight = height;
}

void on_keyevent(GLFWwindow *w, int key, int scancode, int action, int mode)
{

}

void on_text(GLFWwindow *w, unsigned int codepoint)
{

}

void on_cursormove(GLFWwindow *w, double xpos, double ypos)
{
}

void on_cursorbutton(GLFWwindow *w, int button, int action, int mods)
{
}

void on_joystick_conn(int jid, int event)
{
}

void Graphic::Initialize()
{
  Game &game = Game::getInstance();
  sWidth = game.get_window_width();
  sHeight = game.get_window_height();

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
  {
    std::cerr << "Failed to init GLFW." << std::endl;
    exit(EXIT_FAILURE);
  }

  window = glfwCreateWindow(sWidth, sHeight,
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

  // Callback function setting
  // XXX: https://www.glfw.org/docs/latest/input_guide.html
  // TODO: calling callback func is appropriate for other function ...?
  glfwSetWindowSizeCallback(window, on_resize);
  glfwSetKeyCallback(window, on_keyevent);
  glfwSetCharCallback(window, on_text);
  glfwSetCursorPosCallback(window, on_cursormove);
  glfwSetMouseButtonCallback(window, on_cursorbutton);
  glfwSetJoystickCallback(on_joystick_conn);

  // GL flag setting
  glClearColor(0, 0, 0, 1);
  glMatrixMode(GL_PROJECTION);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Graphic::LoopRendering()
{
  while (!glfwWindowShouldClose(window))
  {
    glViewport(0, 0, sWidth, sHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // XXX: "Some" objects might want to be drawn as perspective ...
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    SceneManager::getInstance().Render();
    glFlush();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

void Graphic::Cleanup()
{
  if (window)
  {
    glfwDestroyWindow(window);
    glfwTerminate();
    window = 0;
  }
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
  /* Cleanup function should be called manually ... ! */
  Cleanup();
}

}