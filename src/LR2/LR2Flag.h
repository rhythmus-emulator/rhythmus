#pragma once

#include "Timer.h"
#include <stdint.h>
#include <string>

namespace rhythmus
{

namespace LR2Flag
{
  const std::string &GetText(size_t text_no);
  void HookEvent();
  void UnHookEvent();
}

}