#ifdef WIN32
# include <tchar.h>
# include <Windows.h>
#endif
#include "Graphic.h"
#include "Game.h"
#include "SceneManager.h"
#include <iostream>

using namespace rhythmus;

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

  /**
   * Initialization
   */
  Graphic &graphic = Graphic::getInstance();
  Game &game = Game::getInstance();

  // Load/Set settings first before initialize other objects
  game.LoadOrDefault();
  // TODO: start logger here.
  graphic.Initialize();
  SceneManager::getInstance().Initialize();

  /**
   * Main game loop
   */
  graphic.LoopRendering();

  /**
   * Cleanup
   */
  graphic.Cleanup();
  if (!game.Save())
  {
    std::cerr << "Settings not saved." << std::endl;
  }
  // TODO: exit logger here.

  exit(EXIT_SUCCESS);
}