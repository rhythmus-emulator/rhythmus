#include "Logger.h"
#include "Graphic.h"
#include "Game.h"
#include "Event.h"
#include "Util.h"
#include "Song.h"
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

  Game::Create();

  for (int i = 0; i < argc; i++)
    GAME->LoadArgument(argv[i]);

  Game::Initialize();

  Game::Loop();

  Game::Cleanup();
  
  exit(EXIT_SUCCESS);
}