#include "Setting.h"

namespace rhythmus
{

bool Setting::Open(const std::string& setting_path)
{
  return false;
}

bool Setting::Save()
{
  return false;
}

template<>
void Setting::Load(int& value, const std::string& key)
{
}

template<>
void Setting::Load(std::string& value, const std::string& key)
{
}

}