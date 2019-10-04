#include "Logger.h"
#include "Graphic.h"
#include "Game.h"
#include "Sound.h"
#include "Timer.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "LR2/LR2Flag.h"  // For updating LR2 flag
#include "Event.h"
#include "Util.h"
#include "Song.h"
#include "TaskPool.h"
#include <iostream>

// declare windows header latest due to constant conflicts
#ifdef WIN32
# include <tchar.h>
# include <Windows.h>
#endif

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
    // convert wargv into utf8 string
    argv_str[i] = GetUtf8FromWString(wargv[i]);
    argv[i] = argv_str[i].c_str();
  }
#else
int main(int argc, char **argv)
{
#endif

  // before starting initialization, system path should be cached first.
  ResourceManager::CacheSystemDirectory();

  /**
   * Initialization
   */
  Graphic &graphic = Graphic::getInstance();
  Game &game = Game::getInstance();
  for (int i = 0; i < argc; i++)
    game.LoadArgument(argv[i]);

  // Load/Set settings first before initialize other objects
  game.LoadOrDefault();

  Logger::getInstance().StartLogging();
  Logger::getInstance().HookStdOut();
  TaskPool::getInstance().SetPoolSize(4);
  graphic.Initialize();
  SceneManager::getInstance().Initialize();
  Timer::Initialize();
  SoundDriver::getInstance().Initialize();

  // Event Initialization
  EventManager::Initialize();
  LR2Flag::SubscribeEvent(); // Don't need to do if you won't support LR2 skin

  /**
   * Main game loop
   */
  graphic.LoopRendering();

  /**
   * Cleanup
   */
  
  TaskPool::getInstance().ClearTaskPool();
  SceneManager::getInstance().Cleanup();
  SongPlayable::getInstance().CancelLoad(); // if loading, than cancel
  SongPlayable::getInstance().Clear();
  graphic.Cleanup();
  SoundDriver::getInstance().Destroy();
  if (!game.Save())
  {
    std::cerr << "Settings not saved." << std::endl;
  }
  Logger::getInstance().FinishLogging();

  exit(EXIT_SUCCESS);
}