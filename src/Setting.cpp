#include "Setting.h"
#include "Util.h"
#include <iostream>
#include <sstream>

using namespace tinyxml2;

namespace rhythmus
{

constexpr char* kDefaultRootName = "setting";

SettingRow* SettingList::Get(const std::string& name)
{
  for (auto& ii : *this)
  {
    if (ii.name == name)
      return &ii;
  }
  return nullptr;
}


// ------------------------------ class Setting

Setting::Setting()
  : root_(0)
{
  Close();
}

// @warn  automatically node is cleared when LoadFile is called.
bool Setting::Open(const std::string& setting_path)
{
  path_ = setting_path;
  XMLError errcode;
  if ((errcode = doc_.LoadFile(setting_path.c_str())) != XML_SUCCESS)
  {
    std::cerr << "Game settings reading failed, TinyXml2 ErrorCode: " << errcode << std::endl;
    // insert root node to use Setting class though file opening failed
    root_ = doc_.NewElement(kDefaultRootName);
    doc_.InsertFirstChild(root_);
    return false;
  }
  root_ = doc_.RootElement();
  if (!root_)
  {
    root_ = doc_.NewElement(kDefaultRootName);
    doc_.InsertFirstChild(root_);
  }
  return true;
}

bool Setting::Save()
{
  if (path_.empty()) return false;

  return doc_.SaveFile(path_.c_str()) == XML_SUCCESS;
}

// @info  Root node is always set to add attributes safely.
void Setting::Close()
{
  doc_.Clear();
  path_.clear();
  root_ = doc_.NewElement(kDefaultRootName);
  doc_.InsertFirstChild(root_);
}

void Setting::SetPreferenceGroup(const std::string& preference_name)
{
  root_ = doc_.RootElement();
  if (preference_name.empty())
    return;
  XMLElement* e = root_->FirstChildElement(preference_name.c_str());
  if (!e)
  {
    e = doc_.NewElement(preference_name.c_str());
    root_->InsertEndChild(e);
  }
  root_ = e;
}

bool Setting::Exist(const std::string& key) const
{
  return root_->FirstChildElement(key.c_str()) != nullptr;
}

void Setting::EnumOption(const std::string& key, std::vector<std::string>& options)
{
  tinyxml2::XMLElement *e = root_->FirstChildElement(kSettingFixedPrefName);
  while (e)
  {
    const char* name = e->Attribute("name");
    if (name && strcmp(name, key.c_str()) == 0)
      break;
    e = e->NextSiblingElement(kSettingFixedPrefName);
  }
  if (!e)
    return;
  std::string desc = e->Attribute("desc", "");
  std::string optionstr = e->Attribute("options", "");

  if (desc == "option" || desc == "list")
  {
    Split(optionstr, ',', options);
  }
  else if (desc == "file")
  {
    // TODO ...
  }
}

void Setting::GetAllPreference(SettingList& pref_list)
{
  XMLElement *e = root_->FirstChildElement();
  while (e)
  {
    const char* name = e->Name();
    const char* t = e->GetText();
    t = t ? t : "";
    if (strcmp(name, kSettingFixedPrefName) != 0)
    {
      pref_list.push_back({ name, "", t });
    }
    else
    {
      pref_list.push_back({ name, e->Attribute("desc", ""), t });
    }
    e = e->NextSiblingElement();
  }
}

std::string Setting::ConvertToSafeKey(const std::string& key)
{
  std::string find_key = key;
  if (!(find_key[0] >= 'a' && find_key[0] <= 'z' || find_key[0] >= 'A' && find_key[0] <= 'Z'))
    find_key = "N" + find_key;
  for (int i = 0; i < find_key.size(); i++)
    if (find_key[i] == '/' || find_key[i] == '\\' || find_key[i] == '*' || find_key[i] == ' ')
      find_key[i] = '_';
  return find_key;
}



// ------------------------ Serialization

template <>
void ConvertFromString(const std::string& src, std::string& dst)
{
  dst = src;
}

template <>
void ConvertFromString(const std::string& src, int& dst)
{
  // for LR2 style
  if (src.size() > 0 && src[0] == '!')
    dst = -atoi(src.c_str() + 1);
  else
    dst = atoi(src.c_str());
}

template <>
void ConvertFromString(const std::string& src, double& dst)
{
  dst = atof(src.c_str());
}

template <>
void ConvertFromString(const std::string& src, bool& dst)
{
  dst = (stricmp(src.c_str(), "true") == 0);
}

template <>
void ConvertFromString(const std::string& src, unsigned short& dst)
{
  dst = static_cast<unsigned short>(atoi(src.c_str()));
}

template <>
void ConvertFromString(const std::string& src, std::vector<std::string>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(token);
  }
}

template <>
void ConvertFromString(const std::string& src, std::vector<int>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(atoi(token.c_str()));
  }
}

template <>
void ConvertFromString(const std::string& src, std::vector<double>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(atof(token.c_str()));
  }
}



// ---------------------- Deserialization

template <>
std::string ConvertToString(const std::string& dst)
{
  return dst;
}

template <>
std::string ConvertToString(const int& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const unsigned& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const unsigned short& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const double& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const bool& dst)
{
  return dst ? "true" : "false";
}

template <>
std::string ConvertToString(const std::vector<std::string>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

template <>
std::string ConvertToString(const std::vector<int>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

template <>
std::string ConvertToString(const std::vector<double>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

}