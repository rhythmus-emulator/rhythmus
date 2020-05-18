#pragma once

#include <map>
#include <string>
#include <stddef.h>

namespace rhythmus
{

template <typename T>
class KeyData
{
public:
  KeyData() : name_(nullptr), v_(nullptr) {}
  KeyData(const std::string *name, T* v) : name_(name), v_(v) {}
  const T& get() const { return *v_; }
  T& get() { return *v_; }
  void set(const T& v) { *v_ = v; }
  const std::string &name() { return *name_; }
  T& operator*() { return *v_; }
  const T& operator*() const { return *v_; }
private:
  const std::string *name_;
  T *v_;
};

/**
 * @brief
 * Fast and convinent key-value pool
 */
class KeyPool
{
public:
  KeyPool();

  KeyData<int> GetInt(const std::string &name);
  KeyData<float> GetFloat(const std::string &name);
  KeyData<std::string> GetString(const std::string &name);

private:
  std::map<std::string, int> intpool_;
  std::map<std::string, std::string> strpool_;
  std::map<std::string, float> floatpool_;
};

// Initialized when program starts, so safe to use.
extern KeyPool *KEYPOOL;

}