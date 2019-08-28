#include "Setting.h"
#include <iostream>
#include <vector>
#include <sstream>

using namespace tinyxml2;

namespace rhythmus
{

Setting::Setting()
  : root_(0)
{
  Close();
}

// @warn  automatically node is cleared when LoadFile is called.
bool Setting::Open(const std::string& setting_path)
{
  XMLError errcode;
  if ((errcode = doc_.LoadFile(setting_path.c_str())) != XML_SUCCESS)
  {
    std::cerr << "Game settings reading failed, TinyXml2 ErrorCode: " << errcode << std::endl;
    return false;
  }
  root_ = doc_.RootElement();
  path_ = setting_path;
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
  root_ = doc_.NewElement("setting");
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

template<typename T>
void Setting::Load(const std::string& key, T& value) const
{
  XMLElement *e = root_->FirstChildElement(key.c_str());
  if (!e) return;
  const char* t = e->GetText();
  ConvertFromString(t ? t : "", value);
}

template<typename T>
void Setting::Set(const std::string& key, const T& value)
{
  std::string v = ConvertToString(key);
  XMLElement *e = root_->FirstChildElement(key.c_str());
  if (!e)
  {
    e = doc_.NewElement(key.c_str());
    root_->InsertEndChild(e);
  }
  e->SetText(v.c_str());
}

bool Setting::Exist(const std::string& key) const
{
  return root_->FirstChildElement(key.c_str()) != nullptr;
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
std::string ConvertToString(const double& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const std::vector<std::string>& dst)
{
  std::stringstream ss;
  int i = 0;
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
  int i = 0;
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
  int i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

}