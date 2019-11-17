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

Option::Option(const std::string& name)
  : name_(name), type_("text"),
    number_(0), number_min_(0), number_max_(0x7FFFFFFF),
    save_with_constraint_(false) {}

const std::string& Option::name() const
{
  return name_;
}

const std::string& Option::value() const
{
  return value_;
}

const std::string& Option::type() const
{
  return type_;
}

const std::string& Option::desc() const
{
  return desc_;
}

void Option::set_description(const std::string& desc)
{
  desc_ = desc;
}

const std::string& Option::get_constraint() const
{
  return option_filter_;
}

int Option::value_int() const
{
  return number_min_ + number_;
}

int Option::value_min() const
{
  return number_min_;
}

int Option::value_max() const
{
  return number_max_;
}

bool Option::is_constraint() const
{
  return save_with_constraint_;
}

void Option::Next()
{
  number_ = std::min(number_ + 1, number_max_);

  if (type_ == "number")
  {
    value_ = std::to_string(number_);
  }
  else /* file, option */
  {
    value_ = options_[number_];
  }
}

void Option::Prev()
{
  number_ = std::max(number_ - 1, number_min_);

  if (type_ == "number")
  {
    value_ = std::to_string(number_);
  }
  else /* file, option */
  {
    value_ = options_[number_];
  }
}

void Option::Clear()
{
  type_ = "text";
}

void Option::SetDefault(const std::string &default__)
{
  default_ = default__;
}

void Option::SetTextOption()
{
  type_ = "text";
  option_filter_.clear();
  options_.clear();
}

void Option::SetOption(const std::string& options)
{
  Clear();
  std::string optionstr(options);
  if (options[0] == '!' && options.size() > 2)
  {
    optionstr = optionstr.substr(2);
    if (options[1] == 'N')
    {
      std::vector<std::string> params;
      type_ = "number";
      Split(optionstr, ',', params);
      number_min_ = atoi(params[0].c_str());
      number_max_ = atoi(params[1].c_str());
    }
    else if (options[2] == 'F')
    {
      type_ = "file";
      ResourceManager::getInstance().GetAllPaths(optionstr, options_);
    }
    else
    {
      type_ = "text"; /* invalid? */
      return;
    }
  }
  else
  {
    type_ = "option";
    Split(optionstr, ',', options_);
  }

  type_ = "option";
  option_filter_ = options;
}

void Option::set_value(const std::string& option)
{
  value_ = option;
  number_ = atoi(option.c_str());
  Validate();
}

void Option::reset_value()
{
  set_value(default_);
}

void Option::set_value(int value)
{
  number_ = value;
  value_ = std::to_string(value);
  Validate(true);
}

void Option::save_with_constraint(bool v)
{
  save_with_constraint_ = v;
}

// @brief validate option properly after setting value_.
//        internally called in SetSelected(). for internal use.
void Option::Validate(bool valid_using_number)
{
  if (type_ == "text")
  {
    /* do nothing */
  }
  if (type_ == "number")
  {
    if (number_ > number_max_ || number_ < number_min_)
    {
      // constraint failed, do re-validate
      number_ = std::min(std::max(number_, number_min_), number_max_);
      value_ = std::to_string(number_);
      Validate();
    }
  }
  else /* file, option */
  {
    if (options_.empty())
      return;

    if (!valid_using_number)
    {
      auto iter = std::find(options_.begin(), options_.end(), value_);
      if (iter == options_.end())
      {
        // constraint failed, set it to first value (or default)
        value_ = options_.front();
        number_ = 0;
      }
      else
      {
        number_ = (int)(iter - options_.begin());
      }
    }
    else
    {
      if (options_.empty())
      {
        number_ = 0;
        value_.clear();
        return;
      }
      number_ = std::max(0, std::min(number_, (int)options_.size() - 1));
      value_ = options_[number_];
    }
  }
}

void Option::CopyConstraint(const Option& option)
{
  desc_ = option.desc_;
  value_ = option.value_;
  type_ = option.type_;
  options_ = option.options_;
  option_filter_ = option.option_filter_;
  number_ = option.number_;
  number_min_ = option.number_min_;
  number_max_ = option.number_max_;
}


// --------------------------- class OptionList

OptionGroup::OptionGroup()
{
}

OptionGroup::~OptionGroup()
{
}

Option &OptionGroup::GetOption(const std::string &name)
{
  return options_[name];
}

void OptionGroup::DeleteOption(const std::string &name)
{
  options_.erase(name);
}

bool OptionGroup::Exist(const std::string &name) const
{
  return options_.find(name) != options_.end();
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
  XMLElement *groupnode, *optnode;

  groupnode = root->FirstChildElement("Group");
  while (groupnode)
  {
    const char *grpname = groupnode->Attribute("name");
    if (!grpname)
      continue;
    optnode = groupnode->FirstChildElement("Option");
    while (optnode)
    {
      const char *name = optnode->Attribute("name");
      const char *constraint = optnode->Attribute("constraint");
      const char *desc = optnode->Attribute("desc");
      if (!name)
        continue;

      Option &option = GetGroup(grpname).GetOption(name);
      if (!constraint)
        option.SetTextOption();
      else
        option.SetOption(constraint);
      if (desc)
        option.set_description(desc);

      optnode = optnode->NextSiblingElement("Option");
    }
    groupnode = groupnode->NextSiblingElement("Group");
  }

  return true;
}

void OptionList::LoadFromMetrics(const Metric &metric)
{
  if (!metric.exist("Option"))
    return;
  int optioncount = metric.get<int>("Option");
  std::vector<std::string> params;
  for (int i = 0; i < optioncount; ++i)
  {
    /* groupname : metric name */
    /* attr,default,select list */
    params = std::move(metric.get<decltype(params)>("Option" + std::to_string(i)));
    OptionGroup &group = option_groups_[metric.name()];
    auto &option = group.GetOption(params[0]);
    option.SetDefault(params[1]);
    /* XXX: so dirty code, need to tidy it */
    std::string optstr;
    for (size_t i = 2; i < params.size(); ++i)
      optstr += params[i] + ",";
    if (!optstr.empty()) optstr.pop_back();
    option.SetOption(optstr);
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

  for (auto &group : option_groups_)
  {
    XMLElement *groupnode = root->FirstChildElement(group.first.c_str());
    if (!groupnode)
      continue;
    for (auto &option : group.second)
    {
      XMLElement *valnode = groupnode->FirstChildElement(option.first.c_str());
      if (!valnode)
        continue;
      const char* val = valnode->Attribute("value");
      if (!val)
        continue;
      option.second.set_value(val);
    }
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

  XMLElement *nodegroup, *nodeval;
  for (auto &group : option_groups_)
  {
    std::string groupname(group.first);
    nodegroup = doc.NewElement(groupname.c_str());
    root->InsertFirstChild(nodegroup);
    for (auto &options : group.second)
    {
      nodeval = doc.NewElement(options.first.c_str());
      nodegroup->InsertFirstChild(nodeval);
      nodeval->SetAttribute("value", options.second.value().c_str());
    }
  }

  return doc.SaveFile(path.c_str()) == XML_SUCCESS;
}

OptionGroup &OptionList::GetGroup(const std::string &groupname)
{
  return option_groups_[groupname];
}

OptionGroup &OptionList::DeleteGroup(const std::string &groupname)
{
  option_groups_.erase(groupname);
}

void OptionList::Clear()
{
  option_groups_.clear();
}


// ------------------------- class Metric

Metric::Metric(const std::string &name) : name_(name) {}

Metric::~Metric() {}

void Metric::set_name(const std::string &name) { name_ = name; }

const std::string& Metric::name() const { return name_; }

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

bool MetricList::exist(const std::string &group, std::string &key) const
{
  auto it = metricmap_.find(group);
  if (it == metricmap_.end()) return false;
  return it->second.exist(group);
}

void MetricList::set(const std::string &group, const std::string &key, const std::string &v)
{
  auto it = metricmap_.find(group);
  ASSERT_M(it != metricmap_.end(), "Metric group '" + group + "' is not found.");
  it->second.set(key, v);
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
    "",
    "",
    "",
    0
  };

  // read global setting first, then read metric.
  // after that, read user setting, as it may depends on metrics.
  GetSystemSetting().ReadFromFile(kSettingPath);
  {
    for (size_t i = 0; metric_option_attrs[i]; ++i)
    {

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

OptionList &Setting::GetSystemSetting()
{
  static OptionList sysoptions;
  static bool is_initialized = false;
  if (!is_initialized)
  {
    // TODO: add basic system options here.
#if 0
  /* Initialize game setting constraints */
    {
      Option &option = *setting_.NewOption("Resolution");
      option.set_description("Game resolution.");
      // TODO: get exact information from graphic card
      option.SetOption("640x480,800x600,1280x960,1280x720,1440x900,1600x1050,1920x1200");
    }

    {
      Option &option = *setting_.NewOption("SoundDevice");
      option.set_description("Set default sound device.");
      option.SetOption("");   // empty
    }

    {
      Option &option = *setting_.NewOption("SoundBufferSize");
      option.set_description("Sound latency increases if sound buffer is big. If sound flickers, use large buffer size.");
      option.SetOption("1024,2048,3072,4096,8192,16384");
      option.set_value(2048); // default value
    }

    {
      Option &option = *setting_.NewOption("Volume");
      option.set_description("Set game volume.");
      option.SetOption("0,10,20,30,40,50,60,70,80,90,100");
    }

    {
      Option &option = *setting_.NewOption("GameMode");
      option.set_description("Set game mode, whether to run as arcade or home.");
      option.SetOption("Home,Arcade,LR2");
    }

    {
      Option &option = *setting_.NewOption("Logging");
      option.set_description("For development.");
      option.SetOption("Off,On");
    }

    {
      Option &option = *setting_.NewOption("SelectScene");
      option.set_description("File path of select scene.");
      option.SetFileOption("../themes/*/select/*.lr2skin");
    }

    {
      Option &option = *setting_.NewOption("DecideScene");
      option.set_description("File path of decide scene.");
      option.SetFileOption("../themes/*/decide/*.lr2skin");
    }

    {
      Option &option = *setting_.NewOption("PlayScene");
      option.set_description("File path of play scene.");
      option.SetFileOption("../themes/*/play/*.lr2skin");
    }

    {
      Option &option = *setting_.NewOption("ResultScene");
      option.set_description("File path of result scene.");
      option.SetFileOption("../themes/*/result/*.lr2skin");
    }

    {
      Option &option = *setting_.NewOption("CourseResultScene");
      option.set_description("File path of course result scene.");
      option.SetFileOption("../themes/*/courseresult/*.lr2skin");
    }
#endif
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