/**
 * @brief
 * Xml Setting Load & Save class, Used by Game / SceneManager class.
 * Generally used for game internal setting option.
 * This file also provides Serialization & Deserialization for objects
 * which is necessary for setting save & load.
 *
 * @warn
 * 
 */

#pragma once

#include <string>
#include <vector>
#include "tinyxml2.h"

namespace rhythmus
{

constexpr char* kSettingFixedPrefName = "option";

struct SettingRow
{
  std::string name;
  std::string desc;
  std::string value;
};

class SettingList : public std::vector<SettingRow>
{
public:
  SettingRow* Get(const std::string& name);
};

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

  /* @brief if failed to find specified node, then calls LoadPref internally. */
  template <typename T>
  void Load(const std::string& key, T& value) const
  {
    if (key == kSettingFixedPrefName)
      return; /* reserved name, not saved */
    tinyxml2::XMLElement *e = root_->FirstChildElement(key.c_str());
    if (!e)
    {
      std::string _2, _3, _4;
      LoadOption<T>(key, _2, _3, _4, value);
    }
    const char* t = e->GetText();
    ConvertFromString(t ? t : "", value);
  }

  /* @brief only searchs for SetPref() */
  template <typename T>
  void LoadOption(const std::string& key, std::string& type,
    std::string& desc, std::string& options, T& value) const
  {
    tinyxml2::XMLElement *e = root_->FirstChildElement(kSettingFixedPrefName);
    while (e)
    {
      const char* name = e->Attribute("name");
      if (name && strcmp(name, key.c_str()) == 0)
        break;
      e = e->NextSiblingElement(kSettingFixedPrefName);
    }
    if (!e) return;
    const char* t = e->GetText();
    type = e->Attribute("type", "");
    desc = e->Attribute("desc", "");
    options = e->Attribute("options", "");
    ConvertFromString(t ? t : "", value);
  }

  /* @brief Set setting with node with specified name. */
  template <typename T>
  void Set(const std::string& key, const T& value)
  {
    std::string v = ConvertToString(value);
    tinyxml2::XMLElement *e = root_->FirstChildElement(key.c_str());
    if (!e)
    {
      e = doc_.NewElement(key.c_str());
      root_->InsertEndChild(e);
    }
    e->SetText(v.c_str());
  }

  /**
   * @brief Set setting with pref node, which can store option setting.
   * @param key option key
   * @param desc option description
   * @param type type of option (e.g. number, list, file, ...)
   * @param options selectable list, generally uses file filter.
   * @param value currently selected value
   */
  template <typename T>
  void SetOption(const std::string& key, const std::string& type,
    const std::string& desc, const std::string& options, const T& value)
  {
    std::string v = ConvertToString(value);
    tinyxml2::XMLElement *e = root_->FirstChildElement(kSettingFixedPrefName);
    while (e)
    {
      const char* name = e->Attribute("name");
      if (name && strcmp(name, key.c_str()) == 0)
        break;
      e = e->NextSiblingElement(kSettingFixedPrefName);
    }
    if (!e)
    {
      e = doc_.NewElement(kSettingFixedPrefName);
      e->SetAttribute("name", key.c_str());
      root_->InsertEndChild(e);
    }
    e->SetAttribute("type", type.c_str());
    e->SetAttribute("desc", desc.c_str());
    e->SetAttribute("options", options.c_str());

    e->SetText(v.c_str());
  }

  bool Exist(const std::string& key) const;

  /* @brief get options in case of option setting */
  void EnumOption(const std::string& name, std::vector<std::string>& options);

  void GetAllPreference(SettingList& pref_list);

private:
  std::string path_;
  tinyxml2::XMLDocument doc_;

  // current root of preference keys.
  // set by SetPreferenceGroup.
  tinyxml2::XMLElement* root_;

  /* @depreciated @brief convert key to xml-safe-name one. */
  static std::string ConvertToSafeKey(const std::string& key);
};

template <typename T>
void ConvertFromString(const std::string& src, T& dst);

template <typename T>
std::string ConvertToString(const T& dst);

}