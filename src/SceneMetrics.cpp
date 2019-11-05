#include "SceneMetrics.h"
#include "SceneManager.h"
#include "Util.h"

#include "Sprite.h"
#include "object/Text.h"
#include "LR2/LR2SceneLoader.h" /* for lr2skin file load */

#include <iostream>

namespace rhythmus
{
  
constexpr char* kLR2SubstitutePath = "LR2files/Theme";
constexpr char* kSubstitutePath = "../themes";

// ------------------------- class ThemeMetrics

ThemeMetrics::ThemeMetrics(const std::string &name) : name_(name) {}

ThemeMetrics::~ThemeMetrics() {}

void ThemeMetrics::set_name(const std::string &name) { name_ = name; }

const std::string& ThemeMetrics::name() const { return name_; }

bool ThemeMetrics::exist(const std::string &key) const
{
  auto it = attr_.find(key);
  return (it != attr_.end());
}

bool ThemeMetrics::get(const std::string &key, std::string &v) const
{
  auto it = attr_.find(key);
  if (it == attr_.end()) return false;
  v = it->second;
  return true;
}

bool ThemeMetrics::get(const std::string &key, int &v) const
{
  auto it = attr_.find(key);
  if (it == attr_.end()) return false;
  v = atoi(it->second.c_str());
  return true;
}

bool ThemeMetrics::get(const std::string &key, std::vector<std::string> &v) const
{
  auto it = attr_.find(key);
  if (it == attr_.end()) return false;
  Split(it->second, ',', v);
  return true;
}

bool ThemeMetrics::get(const std::string &key, std::vector<int> &v) const
{
  std::vector<std::string> vtmp;
  if (!get(key, vtmp)) return false;
  for (auto &s : vtmp)
    v.push_back(atoi(s.c_str()));
  return true;
}

ThemeMetrics::iterator ThemeMetrics::begin() { attr_.begin(); }
ThemeMetrics::const_iterator ThemeMetrics::cbegin() { attr_.cbegin(); }
ThemeMetrics::iterator ThemeMetrics::end() { attr_.end(); }
ThemeMetrics::const_iterator ThemeMetrics::cend() { attr_.cend(); }

void LoadMetrics(const std::string &filepath, Scene &scene)
{
  std::string ext = Upper(GetExtension(filepath));
  auto &metrics = SceneManager::getMetricList();

  if (ext == "INI")
  {
    // TODO: Stepmania metrics info
    std::cerr << "Warning: Stepmania skin is not supported yet" << std::endl;
  }
  else if (ext == "LR2SKIN")
  {
    LR2SceneLoader loader;
    loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
    loader.Load(filepath);

    // TODO: convert csv file into metrics

    // TODO: load scene option (default)

    // TODO: load scene option (saved one, if exist)

    // TODO: now all metrics setting is done :
    // set thememetrics, create object, load resource using metrics.

#if 0
    for (auto &v : loader)
    {
      if (v.first == "#ENDOFHEADER")
        break;
      setting_.LoadProperty(v.first, v.second);
    }
    setting_.ValidateAll();

    // open option file (if exists)
    // must do it before loading elements, as option affects to it.
    LoadOptions();

    // now load elements
    for (auto &v : loader)
    {
      LoadProperty(v.first, v.second);
    }
#endif
  }
  else if (ext == "LR2SS")
  {
    // TODO: LR2 soundset file
  }
  else
  {
    std::cerr << "Error: not supported ThemeMetrics file: "
      << filepath << std::endl;
  }
}

// XXX: only for LR2 type?
BaseObject* CreateObjectFromMetrics(ThemeMetrics &metrics)
{
  if (metrics.name() != "CreateLR2Object")
    return nullptr;

  BaseObject *obj = nullptr;
  std::string _typename;
  metrics.get("type", _typename);

  if (_typename == "IMAGE")
    obj = new Sprite();
  else if (_typename == "TEXT")
    obj = new Text();

  if (!obj) return nullptr;

  for (auto &it : metrics)
  {
    obj->LoadProperty(it.first, it.second);
  }

  return obj;
}

}