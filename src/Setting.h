/**
 * @brief
 * Xml Setting Load & Save class, Used by Game / SceneManager class.
 * Generally used for game internal setting option.
 * This file also provides Serialization & Deserialization for objects
 * which is necessary for setting save & load.
 */

#pragma once

#include <string>
#include "tinyxml2.h"

namespace rhythmus
{

/* @brief Xml setting load & save class
 * @warn  Key is not duplicable. */
class Setting
{
public:
  Setting();

  bool Open(const std::string& setting_path);
  bool Save();
  void Close();

  void SetPreferenceGroup(const std::string& preference_name = "");

  template <typename T>
  void Load(const std::string& key, T& value) const;

  template <typename T>
  void Set(const std::string& key, const T& value);

  bool Exist(const std::string& key) const;

private:
  std::string path_;
  tinyxml2::XMLDocument doc_;

  // current root of preference keys.
  // set by SetPreferenceGroup.
  tinyxml2::XMLElement* root_;
};

template <typename T>
void ConvertFromString(const std::string& src, T& dst);

template <typename T>
std::string ConvertToString(const T& dst);

}