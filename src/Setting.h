/**
 * @brief
 * Xml Setting Load & Save class, Used by Game / SceneManager class.
 * Generally used for game internal setting option.
 * This file also provides Serialization & Deserialization for objects
 * which is necessary for setting save & load.
 */

#pragma once

#include <string>
#include <vector>
#include "tinyxml2.h"

namespace rhythmus
{

using SettingList = std::vector<std::pair<std::string, std::string> >;

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
  void Load(const std::string& key, T& value) const
  {
    tinyxml2::XMLElement *e = root_->FirstChildElement(key.c_str());
    if (!e) return;
    const char* t = e->GetText();
    ConvertFromString(t ? t : "", value);
  }

  template <typename T>
  void Set(const std::string& key, const T& value)
  {
    std::string v = ConvertToString(key);
    tinyxml2::XMLElement *e = root_->FirstChildElement(key.c_str());
    if (!e)
    {
      e = doc_.NewElement(key.c_str());
      root_->InsertEndChild(e);
    }
    e->SetText(v.c_str());
  }

  bool Exist(const std::string& key) const;

  void GetAllPreference(SettingList& pref_list);

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