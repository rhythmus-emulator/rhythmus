#pragma once

#include <string>

namespace rhythmus
{

class Setting
{
public:
  bool Open(const std::string& setting_path);
  bool Save();

  template <typename T>
  void Load(T& value, const std::string& key);

private:

};

}