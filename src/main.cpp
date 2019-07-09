#ifdef WIN32
# include <tchar.h>
# include <Windows.h>
#endif
#include <GLFW/glfw3.h>
#include <iostream>
#include "Game.h"

static void error_callback(int error, const char* description)
{
  std::cerr << "Error: " << description << std::endl;
}

#if WIN32
int APIENTRY _tWinMain(
  __in HINSTANCE hInstance,
  __in HINSTANCE hPrevInstance,
  __in LPTSTR lpCmdLine,
  __in int nCmdShow)
{
  int argc = __argc;
  wchar_t** wargv = __wargv;
  std::string argv_str[256];
  const char* argv[256];
  memset(argv, 0, sizeof(argv));
  for (int i = 0; i < argc && i < 256; i++)
  {
    // TODO: convert wargv into utf8 string
  }
#else
int main(int argc, char **argv)
{
#endif

  GLFWwindow* window;
  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
  {
    std::cerr << "Failed to init GLFW." << std::endl;
    exit(EXIT_FAILURE);
  }

  Game& g = Game::getInstance();
  if (!g.Load())
  {
    std::cerr << "Failed to load game setting file. use default settings." << std::endl;
  }

  window = glfwCreateWindow(
    g.get_window_width(),
    g.get_window_height(),
    "Hello world!", NULL, NULL
  );
  if (!window)
  {
    std::cerr << "Failed to create window." << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  int fwidth, fheight;
  while (!glfwWindowShouldClose(window))
  {
    glfwGetFramebufferSize(window, &fwidth, &fheight);
    glViewport(0, 0, fwidth, fheight);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}