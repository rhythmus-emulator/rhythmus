#include "Setting.h"
#include "Util.h"
#include "ResourceManager.h"
#include "LR2/LR2SceneLoader.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;

namespace rhythmus
{
  
constexpr const char* kSettingPath = "../config/system.xml";
constexpr const char* kThemeSettingPath = "../config/theme.xml";
constexpr char* kDefaultRootTagName = "setting";

inline const char* GetSafeString(const char* p)
{
  return p ? p : "";
}

// ----------------------------- class Option

Option::Option()
  : type_("option"), curr_sel_idx_(0),
    save_with_constraint_(false) {}

template <> const std::string& Option::value() const
{ return value_; }
template <> const char* Option::value() const
{ return value_.c_str(); }
template <> int Option::value() const
{ return atoi(value_.c_str()); }
template <> size_t Option::value() const
{ return (size_t)atoi(value_.c_str()); }
template <> double Option::value() const
{ return atof(value_.c_str()); }
template <> float Option::value() const
{ return atof(value_.c_str()); }
template <> bool Option::value() const
{ return value_ == "true"; }

const std::string& Option::type() const
{
  return type_;
}

const std::string& Option::desc() const
{
  return desc_;
}

Option &Option::set_description(const std::string& desc)
{
  desc_ = desc;
  return *this;
}

const std::string& Option::get_constraint() const
{
  return constraint_;
}

void Option::Next()
{
  if (options_.empty())
    return;
  if (curr_sel_idx_ + 1 >= options_.size())
    return;
  value_ = options_[++curr_sel_idx_];
}

void Option::Prev()
{
  if (options_.empty())
    return;
  if (curr_sel_idx_ == 0)
    return;
  value_ = options_[--curr_sel_idx_];
}

void Option::Clear()
{
  options_.clear();
  constraint_.clear();
}

Option &Option::SetDefault(const std::string &default__)
{
  default_ = default__;
  return *this;
}

void Option::SetOption(const std::string& options)
{
  constraint_ = options;
  Split(options, ',', options_);
}

void Option::set_value(const std::string& option)
{
  value_ = option;
}

void Option::set_value(int value)
{
  set_value(std::to_string(value));
}

void Option::reset_value()
{
  set_value(default_);
}

void Option::save_with_constraint(bool v)
{
  save_with_constraint_ = v;
}

Option* Option::CreateOption(const std::string &optionstr)
{
  Option *option = nullptr;
  if (optionstr.size() > 2 && optionstr[0] == '#')
  {
    if (optionstr[1] == 'F')
    {
      // create as file option
      option = new FileOption();
    }
    else if (optionstr[1] == 'N')
    {
      // create as number option
      option = new NumberOption();
    }
  }
  else
    option = new Option();

  if (option)
    option->SetOption(optionstr);

  return option;
}

FileOption::FileOption() {}

void FileOption::SetOption(const std::string &options)
{
  ResourceManager::getInstance().GetAllPaths(options, options_);
}

TextOption::TextOption() {}

void TextOption::Next() {}
void TextOption::Prev() {}
void TextOption::set_value(const std::string& value) { value_ = value; }

NumberOption::NumberOption()
  : number_(0), number_min_(0), number_max_(0x7FFFFFFF)
{
  type_ = "number";
}

int NumberOption::value_int() const
{
  return number_min_ + number_;
}

int NumberOption::value_min() const
{
  return number_min_;
}

int NumberOption::value_max() const
{
  return number_max_;
}

void NumberOption::SetOption(const std::string& options)
{
  std::string smin, smax;
  Split(options, ',', smin, smax);
  number_min_ = atoi(smin.c_str());
  number_max_ = atoi(smax.c_str());
}

void NumberOption::Next() { number_ = std::min(number_max_, number_ + 1); }
void NumberOption::Prev() { number_ = std::max(number_min_, number_ - 1); }

void NumberOption::set_value(const std::string& value)
{
  set_value(atoi(value.c_str()));
}

void NumberOption::set_value(int value)
{
  if (value > number_max_) value = number_max_;
  if (value < number_min_) value = number_min_;
  number_ = value;
  Option::set_value(std::to_string(number_));
}



// --------------------------- class OptionList

OptionList::OptionList() {}

OptionList::~OptionList()
{
  Clear();
}

bool OptionList::LoadFromFile(const std::string &filepath)
{
  std::string ext = GetExtension(filepath);
  if (ext != "xml")
  {
    std::cerr << "Error: Only xml option file can be read." << std::endl;
    return;
  }

  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(filepath.c_str()) != XML_SUCCESS)
    return false;
  XMLElement *root = doc.RootElement();
  XMLElement *optnode;

  optnode = root->FirstChildElement("Option");
  while (optnode)
  {
    const char *name = optnode->Attribute("name");
    const char *constraint = optnode->Attribute("constraint");
    const char *desc = optnode->Attribute("desc");
    if (!name)
      continue;

    auto &option = SetOption(name, constraint);
    if (desc)
      option.set_description(desc);

    optnode = optnode->NextSiblingElement("Option");
  }

  return true;
}

void OptionList::LoadFromMetrics(const Metric &metric)
{
  if (!metric.exist("Option"))
    return;
  int optioncount = metric.get<int>("Option");
  for (int i = 0; i < optioncount; ++i)
  {
    /* attr,default,select list */
    CommandArgs args(metric.get<std::string>("Option" + std::to_string(i)), 3);
    auto &option = SetOption(args.Get<std::string>(0), args.Get<std::string>(2));
    option.SetDefault(args.Get<std::string>(1));
  }
}

/* @brief read only option value from file */
bool OptionList::ReadFromFile(const std::string &filepath)
{
  // currently only supports xml type file.
  std::string ext = GetExtension(filepath);
  if (ext != "xml")
  {
    std::cerr << "Error: Only xml option file can be read." << std::endl;
    return false;
  }

  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(filepath.c_str()) != XML_SUCCESS)
    return false;
  XMLElement *root = doc.RootElement();

  for (auto &it : options_)
  {
    auto &opt = *it.second;
    XMLElement *valnode = root->FirstChildElement(it.first.c_str());
    if (!valnode)
      continue;
    const char* val = valnode->Attribute("value");
    if (!val)
      continue;
    opt.set_value(val);
  }

  return true;
}

/* @brief save only option value to file */
bool OptionList::SaveToFile(const std::string &path)
{
  if (path.empty()) return false;
  tinyxml2::XMLDocument doc;
  XMLError errcode;
  XMLElement *root = doc.NewElement(kDefaultRootTagName);
  doc.InsertFirstChild(root);

  XMLElement *nodeval;
  for (auto &it : options_)
  {
    auto &opt = *it.second;
    nodeval = doc.NewElement(it.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", opt.value<std::string>().c_str());
  }

  return doc.SaveFile(path.c_str()) == XML_SUCCESS;
}

Option *OptionList::GetOption(const std::string &key) const
{
  auto it = options_.find(key);
  if (it != options_.end())
    return it->second;
  else
    return nullptr;
}

Option &OptionList::SetOption(const std::string &key, const std::string &optionstr)
{
  return SetOption(key, Option::CreateOption(optionstr));
}

Option &OptionList::SetOption(const std::string &key, Option *option)
{
  auto &it = options_.find(key);
  if (it == options_.end())
  {
    options_[key] = option;
  }
  else
  {
    delete it->second;
    it->second = option;
  }
  return *option;
}

void OptionList::DeleteOption(const std::string &key)
{
  auto it = options_.find(key);
  if (it == options_.end()) return;
  delete it->second;
  options_.erase(it);
}

void OptionList::Clear()
{
  for (auto &it : options_)
    delete it.second;
  options_.clear();
}


// ------------------------- class Metric

Metric::Metric() {}

Metric::~Metric() {}

bool Metric::exist(const std::string &key) const
{
  auto it = attr_.find(key);
  return (it != attr_.end());
}

template <>
std::string Metric::get(const std::string &key) const
{
  auto it = attr_.find(key);
  ASSERT_M(it != attr_.end(), "ThemeMetric Key not found: " + key);
  return it->second;
}

template <>
bool Metric::get(const std::string &key) const
{
  return get<std::string>(key) == "true";
}

template <>
int Metric::get(const std::string &key) const
{
  return atoi(get<std::string>(key).c_str());
}

template <>
double Metric::get(const std::string &key) const
{
  return atof(get<std::string>(key).c_str());
}

template <>
std::vector<std::string> Metric::get(const std::string &key) const
{
  std::vector<std::string> vtmp;
  Split(get<std::string>(key), ',', vtmp);
  return vtmp;
}

template <>
std::vector<int> Metric::get(const std::string &key) const
{
  std::vector<int> vtmp;
  for (auto &s : get<std::vector<std::string> >(key))
  {
    vtmp.push_back(atoi(s.c_str()));
  }
  return vtmp;
}

void Metric::set(const std::string &key, const std::string &v)
{
  attr_[key] = v;
}

void Metric::set(const std::string &key, int v)
{
  set(key, std::to_string(v));
}

void Metric::set(const std::string &key, double v)
{
  set(key, std::to_string(v));
}

void Metric::set(const std::string &key, bool v)
{
  set(key, v ? "true" : "false");
}

Metric::iterator Metric::begin() { attr_.begin(); }
Metric::const_iterator Metric::cbegin() const { attr_.cbegin(); }
Metric::iterator Metric::end() { attr_.end(); }
Metric::const_iterator Metric::cend() const { attr_.cend(); }


void MetricList::ReadMetricFromFile(const std::string &filepath)
{
  std::string ext = Upper(GetExtension(filepath));

  if (ext == "INI")
  {
    // TODO: Stepmania metrics info
    std::cerr << "Warning: Stepmania skin is not supported yet" << std::endl;
  }
  else if (ext == "LR2SKIN")
  {
    ReadLR2Metric(filepath);
  }
  else if (ext == "LR2SS")
  {
    ReadLR2SS(filepath);
  }
  else
  {
    std::cerr << "Error: not supported ThemeMetrics file: "
      << filepath << std::endl;
  }
}

void MetricList::Clear()
{
  metricmap_.clear();
}

constexpr char* kLR2SubstitutePath = "LR2files/Theme";
constexpr char* kSubstitutePath = "../themes";

void MetricList::ReadLR2Metric(const std::string &filepath)
{
  LR2SceneLoader loader;
  std::vector<std::string> params;
  size_t optioncount = 0, imagecount = 0, fontcount = 0;
  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

  Metric *curr_metrics = nullptr;

  /* Set metric group using path
   * (kind of trick; no group information is given at file) */
  std::string group;
  if (filepath.find("select") != std::string::npos)
    group = "SceneSelect";
  else if (filepath.find("decide") != std::string::npos)
    group = "SceneDecide";
  else if (filepath.find("play") != std::string::npos)
    group = "ScenePlay";
  else if (filepath.find("courseresult") != std::string::npos) /* must courseresult first */
    group = "SceneCourseResult";
  else if (filepath.find("result") != std::string::npos)
    group = "SceneResult";

  if (group.empty())
  {
    std::cerr << "Unknown type of LR2 metric file: " << filepath << std::endl;
    return;
  }

  // convert csv commands into metrics
  for (auto &v : loader)
  {
    if (v.first[0] != '#')
      continue;

    /* We're going to load resource information into metrics,
     * So ignore #ENDOFHEADER sign to read them all. */
     //if (v.first == "#ENDOFHEADER")
     //  break;

     /* Get metric name from LR2 command */
    std::string lr2name = Upper(v.first.substr(1));
    std::string value(v.second); 

    /* Set metric attribute */
    if (lr2name == "INFORMATION")
    {
      MakeParamCountSafe(value, params, 4);
      curr_metrics->set("Gamemode", params[0]);
      curr_metrics->set("Title", params[1]);
      curr_metrics->set("Artist", params[2]);
      curr_metrics->set("PreviewImage", params[3]);
    } else if (lr2name == "CUSTOMOPTION")
    {
      curr_metrics->set("Option", (int)optioncount);
      curr_metrics->set("Option" + std::to_string(optioncount), value);
      optioncount++;
    }
    else if (lr2name == "CUSTOMFILE")
    {
      curr_metrics->set("Option", (int)optioncount);
      curr_metrics->set("Option" + std::to_string(optioncount), value);
      optioncount++;
    }
    else if (lr2name == "IMAGE")
    {
      curr_metrics->set("Image", (int)imagecount);
      curr_metrics->set("Image" + std::to_string(imagecount), value);
      imagecount++;
    }
    else if (lr2name == "LR2FONT")
    {
      curr_metrics->set("Font", (int)fontcount);
      curr_metrics->set("Font" + std::to_string(fontcount), value);
      fontcount++;
    }
    else if (lr2name == "FONT")
    {
      // TODO: fallback FONT ?
    }
    else if (lr2name == "BAR_CENTER" || lr2name == "BAR_AVAILABLE")
    {
      curr_metrics->set("MenuCenter", GetFirstParam(value));
    }
    else if (lr2name == "TRANSCLOLR")
    {
      curr_metrics->set("TransparentColor", value);
    }
    else if (lr2name == "STARTINPUT" || lr2name == "IGNOREINPUT")
    {
      curr_metrics->set("InputStart", GetFirstParam(value));
    }
    else if (lr2name == "FADEOUT")
    {
      curr_metrics->set("FadeOut", GetFirstParam(value));
    }
    else if (lr2name == "FADEIN")
    {
      curr_metrics->set("FadeIn", GetFirstParam(value));
    }
    else if (lr2name == "SCENETIME")
    {
      curr_metrics->set("Timeout", GetFirstParam(value));
    }
    else if (lr2name == "HELPFILE")
    {
      // TODO
    }
  }
}

void MetricList::ReadLR2SS(const std::string &filepath)
{
  LR2SceneLoader loader;
  loader.SetSubStitutePath("LR2files", "..");
  loader.Load(filepath);

#define NAMES \
  NAME("SELECT", "SongSelect"), \
  NAME("DECIDE", "SongDecide"), \
  NAME("EXSELECT", "SongExSelect"), \
  NAME("EXDECIDE", "SongExDecide"), \
  NAME("FOLDER_OPEN", "SongFolderOpen"), \
  NAME("FOLDER_CLOSE", "SongFolderClose"), \
  NAME("PANEL_OPEN", "PanelOpen"), \
  NAME("PANEL_CLOSE", "PanelClose"), \
  NAME("OPTION_CHANGE", "ChangeOption"), \
  NAME("DIFFICULTY", "ChangeDifficulty"), \
  NAME("SCREENSHOT", "Screenshot"), \
  NAME("CLEAR", "Clear"), \
  NAME("FAIL", "Fail"), \
  NAME("MINE", "MineNote"), \
  NAME("SCRATCH", "SongSelectChange"), \
  NAME("COURSECLEAR", "CourseClear"), \
  NAME("COURSEFAIL", "CourseFail")

#define NAME(lr2name, metricname) lr2name
  static const char *_lr2names[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname) metricname
  static const char *_metricnames[] = {
    NAMES, 0
  };
#undef NAME

#undef NAMES

  Metric &metric = metricmap_["Sound"];

  for (auto &v : loader)
  {
    size_t metric_idx = 0;
    std::string lr2name;
    std::string metricname;
    if (v.first[0] != '#')
      continue;
    lr2name = v.first.substr(1);

    std::string vv = Substitute(
      GetFirstParam(v.second), "LR2files", ".."
    );

    while (_lr2names[metric_idx])
    {
      if (lr2name == _lr2names[metric_idx])
        break;
      metric_idx++;
    }

    if (_metricnames[metric_idx] == 0)
      continue;
    metricname = _metricnames[metric_idx];
    metric.set(metricname, vv);
  }
}

bool MetricList::exist(const std::string &group, const std::string &key) const
{
  auto it = metricmap_.find(group);
  if (it == metricmap_.end()) return false;
  return it->second.exist(group);
}

Metric *MetricList::get_metric(const std::string &group)
{
  auto it = metricmap_.find(group);
  if (it == metricmap_.end()) return nullptr;
  else return &it->second;
}

void MetricList::set(const std::string &group, const std::string &key, const std::string &v)
{
  metricmap_[group].set(key, v);
}

void MetricList::set(const std::string &group, const std::string &key, int v)
{
  set(group, key, std::to_string(v));
}

void MetricList::set(const std::string &group, const std::string &key, double v)
{
  set(group, key, std::to_string(v));
}

void MetricList::set(const std::string &group, const std::string &key, bool v)
{
  set(group, key, v ? "true" : "false");
}


// ------------------------------ class Setting

void Setting::ReadAll()
{
  static char *metric_option_attrs[] = {
    "ThemeFile",
    "SelectSceneFile",
    "DecideSceneFile",
    "PlaySceneFile",
    "ResultSceneFile",
    "CourseResultSceneFile",
    0
  };

  // read global setting first, then read metric.
  // after that, read user setting, as it may depends on metrics.
  GetSystemSetting().ReadFromFile(kSettingPath);
  {
    std::string filepath;
    for (size_t i = 0; metric_option_attrs[i]; ++i)
    {
      auto *option = GetSystemSetting().GetOption(metric_option_attrs[i]);
      if (!option) continue;
      GetThemeMetricList().ReadMetricFromFile(option->value<std::string>());
    }
  }
  GetThemeSetting().ReadFromFile(kThemeSettingPath);
}

void Setting::ClearAll()
{
  GetThemeMetricList().Clear();
  GetThemeSetting().Clear();
  GetSystemSetting().Clear();
}

void Setting::ReloadAll()
{
  ClearAll();
  ReadAll();
}

void Setting::SaveAll()
{
  if (!GetSystemSetting().SaveToFile(kSettingPath))
    std::cerr << "Failed to save Game setting." << std::endl;
  if (!GetThemeSetting().SaveToFile(kThemeSettingPath))
    std::cerr << "Failed to save Theme setting." << std::endl;
}

OptionList &Setting::GetSystemSetting()
{
  static OptionList sysoptions;
  static bool is_initialized = false;
  if (!is_initialized)
  {
    is_initialized = true;

    /* Initialize game settings */
    sysoptions.SetOption("Resolution", "640x480,800x600,1280x960,1280x720,1440x900,1600x1050,1920x1200")
      .SetDefault("640x480")
      .set_description("Game resolution")
      .reset_value();

    sysoptions.SetOption("SoundDevice", "Default")
      .SetDefault("Default")
      .set_description("Set default sound device.")
      .reset_value();

    sysoptions.SetOption("SoundBufferSize", "1024,2048,3072,4096,8192,16384")
      .SetDefault("2048")
      .set_description("Sound latency increases if sound buffer is big. If sound flickers, use large buffer size.")
      .reset_value();

    sysoptions.SetOption("Volume", "0,10,20,30,40,50,60,70,80,90,100")
      .SetDefault("70")
      .set_description("Set game volume.")
      .reset_value();

    sysoptions.SetOption("GameMode", "Home,Arcade,LR2")
      .SetDefault("LR2")
      .set_description("Set game mode, whether to run as arcade or home.")
      .reset_value();

    sysoptions.SetOption("Logging", "Off,On")
      .SetDefault("On")
      .set_description("For development.")
      .reset_value();

    sysoptions.SetOption("SoundSet", "!F../sound/*.lr2ss")
      .set_description("Soundset file.")
      .reset_value();

    sysoptions.SetOption("Theme", "")
      .set_description("Theme path.")
      .reset_value();

    sysoptions.SetOption("SelectScene", "!F../themes/*/select/*.lr2skin")
      .set_description("Theme path of select scene.")
      .reset_value();

    sysoptions.SetOption("DecideScene", "!F../themes/*/decide/*.lr2skin")
      .set_description("Theme path of decide scene.")
      .reset_value();

    sysoptions.SetOption("PlayScene", "!F../themes/*/play/*.lr2skin")
      .set_description("Theme path of play scene.")
      .reset_value();

    sysoptions.SetOption("ResultScene", "!F../themes/*/result/*.lr2skin")
      .set_description("Theme path of result scene.")
      .reset_value();

    sysoptions.SetOption("CourseResultScene", "!F../themes/*/courseresult/*.lr2skin")
      .set_description("Theme path of courseresult scene.")
      .reset_value();
  }

  return sysoptions;
}

OptionList &Setting::GetThemeSetting()
{
  static OptionList themeoptions;
  return themeoptions;
}

MetricList &Setting::GetThemeMetricList()
{
  static MetricList metriclist;
  return metriclist;
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
void ConvertFromString(const std::string& src, size_t& dst)
{
  dst = static_cast<size_t>(atoi(src.c_str()));
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