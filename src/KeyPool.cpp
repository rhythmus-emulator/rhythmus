#include "KeyPool.h"
#include "Error.h"
#include "config.h"

namespace rhythmus
{

KeyPool::KeyPool()
{
  *GetInt("true") = 1;
  *GetInt("false") = 0;
}


KeyData<int> KeyPool::GetInt(const std::string &name)
{
  R_ASSERT(intpool_.size() < 100000);
  auto ii = intpool_.find(name);
  if (ii == intpool_.end())
  {
    intpool_[name] = 0;
    ii = intpool_.find(name);
  }
  return KeyData<int>(&ii->first, &ii->second);
}

KeyData<float> KeyPool::GetFloat(const std::string &name)
{
  R_ASSERT(floatpool_.size() < 100000);
  auto ii = floatpool_.find(name);
  if (ii == floatpool_.end())
  {
    floatpool_[name] = 0;
    ii = floatpool_.find(name);
  }
  return KeyData<float>(&ii->first, &ii->second);
}

KeyData<std::string> KeyPool::GetString(const std::string &name)
{
  R_ASSERT(strpool_.size() < 100000);
  auto ii = strpool_.find(name);
  if (ii == strpool_.end())
  {
    strpool_[name] = std::string();
    ii = strpool_.find(name);
  }
  return KeyData<std::string>(&ii->first, &ii->second);
}

KeyPool _KEYPOOL;
KeyPool *KEYPOOL = &_KEYPOOL;

}