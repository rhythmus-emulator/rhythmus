#ifdef WIN32
# include <tchar.h>
# include <Windows.h>
#endif
#include "Logger.h"
#include "Graphic.h"
#include "Game.h"
#include "Sound.h"
#include "Timer.h"
#include "SceneManager.h"
#include "LR2/LR2Flag.h"  // For updating LR2 flag
#include "Event.h"
#include "rutil.h"        // convert wargv
#include "./Song.h"
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
    // convert wargv into utf8 string
    rutil::EncodeFromWStr(wargv[i], argv_str[i], rutil::E_UTF8);
    argv[i] = argv_str[i].c_str();
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
  for (int i = 0; i < argc; i++)
    game.LoadArgument(argv[i]);

  // Load/Set settings first before initialize other objects
  game.LoadOrDefault();

  Logger::getInstance().StartLogging();
  Logger::getInstance().HookStdOut();
  graphic.Initialize();
  Game::getInstance().ChangeGameMode();
  Game::getInstance().Update();
  SceneManager::getInstance().Initialize();
  Timer::Initialize();
  GameMixer::getInstance().Initialize();

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
  
  SceneManager::getInstance().Cleanup();
  SongPlayable::getInstance().CancelLoad(); // if loading, than cancel
  SongPlayable::getInstance().Clear();
  graphic.Cleanup();
  GameMixer::getInstance().Destroy();
  if (!game.Save())
  {
    std::cerr << "Settings not saved." << std::endl;
  }
  Logger::getInstance().FinishLogging();

  exit(EXIT_SUCCESS);
}