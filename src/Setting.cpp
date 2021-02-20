#include "Setting.h"
#include "Util.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "LR2/LR2SceneLoader.h"
#include "Script.h"
#include "common.h"
#include "tinyxml2.h"
#include <sstream>

using namespace tinyxml2;

namespace rhythmus
{
  
constexpr const char* kSettingPath = "config/system.xml";
constexpr const char* kDefaultRootTagName = "setting";

inline const char* GetSafeString(const char* p)
{
  return p ? p : "";
}


// ------------------------- class Metric

MetricGroup::MetricGroup() {}

MetricGroup::MetricGroup(const std::string &name) : name_(name) {}

MetricGroup::~MetricGroup() {}

const std::string &MetricGroup::name() const
{
  return name_;
}

void MetricGroup::set_name(const std::string &name)
{
  name_ = name;
}

bool MetricGroup::Load(const std::string &path)
{
  if (path.empty())
  {
    Logger::Error("Error: attempt to load empty metric file");
    return false;
  }

  clear();
  std::string ext = Upper(GetExtension(path));

  if (ext == "INI")
  {
    return LoadFromIni(path);
  }
  else if (ext == "XML")
  {
    return LoadFromXml(path);
  }

  Logger::Error("Error: not supported Metrics file: %s", path.c_str());
  return false;
}

bool MetricGroup::LoadFromXml(const std::string &path)
{
  tinyxml2::XMLDocument doc;
  std::list<XMLElement *> node_to_load;
  std::list<MetricGroup *> group_to_load;

  if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
  {
    //throw FileNotFoundException(path);
    return false;
  }

  XMLElement* root = doc.RootElement();
  XMLElement* ele;
  MetricGroup *g;
  node_to_load.push_back(root->FirstChildElement());
  group_to_load.push_back(this);

  const char *name = root->Attribute("name");
  if (name) set_name(name);

  while (!node_to_load.empty())
  {
    ele = node_to_load.back();
    g = group_to_load.back();
    node_to_load.pop_back();
    group_to_load.pop_back();

    while (ele)
    {
      const char* elename = ele->Name();
      MetricGroup& metric = g->add_group(elename);
      const XMLAttribute* attr = ele->FirstAttribute();
      while (attr)
      {
        const char* v = attr->Value();
        R_ASSERT_M(v, "No value given on xml attribute.");
        metric.set(attr->Name(), attr->Value());
        attr = attr->Next();
      }

      node_to_load.push_back(ele->FirstChildElement());
      group_to_load.push_back(&metric);

      ele = ele->NextSiblingElement();
    }
  }

  return true;
}

bool MetricGroup::LoadFromIni(const std::string &path)
{
  // TODO: Stepmania metrics info
  throw UnimplementedException("Stepmania skin is not supported yet.");
}

bool MetricGroup::Save(const std::string &path)
{
  // @warn  always saved as xml.
  if (path.empty()) return false;
  tinyxml2::XMLDocument doc;
  XMLElement *root = doc.NewElement(kDefaultRootTagName);
  doc.InsertFirstChild(root);

  std::vector<MetricGroup *> m_stack;
  std::vector<XMLElement *> node_stack;
  m_stack.push_back(this);
  node_stack.push_back(root);

  while (!m_stack.empty())
  {
    MetricGroup *m = m_stack.back();
    XMLElement *n = node_stack.back();
    if (m->name().empty()) continue;
    m_stack.pop_back();
    node_stack.pop_back();

    XMLElement *currnode = doc.NewElement(m->name().c_str());
    n->InsertEndChild(currnode);

    // write attributes to new node
    for (auto i : attr_)
      currnode->SetAttribute(i.first.c_str(), i.second.c_str());

    // push children nodes to stack
    for (auto &c : m->children_)
    {
      m_stack.push_back(&c);
      node_stack.push_back(currnode);
    }
  }

  return true;
}

bool MetricGroup::exist(const std::string &key) const
{
  auto it = attr_.find(key);
  return (it != attr_.end());
}

void MetricGroup::clear()
{
  children_.clear();
  attr_.clear();
}

MetricGroup& MetricGroup::add_group(const std::string &group_name)
{
  MetricGroup* g = get_group(group_name);
  if (g) return *g;
  children_.emplace_back(MetricGroup(group_name));
  return children_.back();
}

MetricGroup* MetricGroup::get_group(const std::string &group_name)
{
  size_t p = group_name.find_first_of('.');
  if (p != std::string::npos)
  {
    std::string g1, g2;
    g1 = group_name.substr(0, p);
    g2 = group_name.substr(p + 1);
    const MetricGroup *group = get_group(g1);
    if (group) group->get_group(g2);
    else return nullptr;
  }
  for (auto ii = children_.rbegin(); ii != children_.rend(); ++ii)
    if (ii->name() == group_name) return &*ii;
  return nullptr;
}

const MetricGroup* MetricGroup::get_group(const std::string &group_name) const
{
  return const_cast<MetricGroup*>(this)->get_group(group_name);
}

const std::string &MetricGroup::get_str(const std::string &key) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get_str(subkeyname);
  }
  auto it = attr_.find(key);
  R_ASSERT_M(it != attr_.end(), "ThemeMetric Key not found: " + key);
  return it->second;
}

template <>
bool MetricGroup::get(const std::string &key) const
{
  const_cast<MetricGroup*>(this)->resolve_fallback(key);
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get<bool>(subkeyname);
  }
  return get_str(key) == "true";
}

template <>
int MetricGroup::get(const std::string &key) const
{
  const_cast<MetricGroup*>(this)->resolve_fallback(key);
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get<int>(subkeyname);
  }
  return atoi(get_str(key).c_str());
}

template <>
size_t MetricGroup::get(const std::string &key) const
{
  const_cast<MetricGroup*>(this)->resolve_fallback(key);
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get<size_t>(subkeyname);
  }
  return (size_t)get<int>(key);
}

template <>
double MetricGroup::get(const std::string &key) const
{
  const_cast<MetricGroup*>(this)->resolve_fallback(key);
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get<double>(subkeyname);
  }
  return atof(get_str(key).c_str());
}

template <>
float MetricGroup::get(const std::string &key) const
{
  const_cast<MetricGroup*>(this)->resolve_fallback(key);
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    R_ASSERT(group);
    return group->get<float>(subkeyname);
  }
  return (float)get<double>(key);
}

#if 0
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
#endif

bool MetricGroup::get_safe(const std::string &key, std::string &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = it->second;
  return it != attr_.end();
}

bool MetricGroup::get_safe(const std::string &key, int &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = atoi(it->second.c_str());
  return it != attr_.end();
}

bool MetricGroup::get_safe(const std::string &key, unsigned &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = (unsigned)atoi(it->second.c_str());
  return it != attr_.end();
}

bool MetricGroup::get_safe(const std::string &key, double &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = atof(it->second.c_str());
  return it != attr_.end();
}

bool MetricGroup::get_safe(const std::string &key, float &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = (float)atof(it->second.c_str());
  return it != attr_.end();
}

bool MetricGroup::get_safe(const std::string &key, bool &v) const
{
  size_t p = key.find_last_of('.');
  if (p != std::string::npos)
  {
    std::string subgroupname, subkeyname;
    subgroupname = key.substr(0, p);
    subkeyname = key.substr(p + 1);
    const MetricGroup *group = get_group(subgroupname);
    if (group) return group->get_safe(subkeyname, v);
    else return false;
  }
  auto it = attr_.find(key);
  if (it != attr_.end())
    v = (Upper(it->second) == "TRUE");
  return it != attr_.end();
}

void MetricGroup::SetFromText(const std::string &metric_text)
{
  size_t ia = 0, ib = 0;
  std::string cmd_type, value;
  while (ib < metric_text.size())
  {
    if (metric_text[ib] == ';' || metric_text[ib] == 0)
    {
      Split(metric_text.substr(ia, ib - ia), ':', cmd_type, value);
      set(cmd_type, value);
      ia = ib = ib + 1;
    }
    else ++ib;
  }
}

void MetricGroup::set(const std::string &key, const std::string &v)
{
  attr_[key] = v;
}

void MetricGroup::set(const std::string &key, const char* v)
{
  attr_[key] = v;
}

void MetricGroup::set(const std::string &key, int v)
{
  set(key, std::to_string(v));
}

void MetricGroup::set(const std::string &key, double v)
{
  set(key, std::to_string(v));
}

void MetricGroup::set(const std::string &key, bool v)
{
  set(key, std::string(v ? "true" : "false"));
}

MetricGroup::iterator MetricGroup::begin() { return attr_.begin(); }
MetricGroup::const_iterator MetricGroup::cbegin() const { return attr_.cbegin(); }
MetricGroup::iterator MetricGroup::end() { return attr_.end(); }
MetricGroup::const_iterator MetricGroup::cend() const { return attr_.cend(); }

MetricGroup::group_iterator MetricGroup::group_begin() { return children_.begin(); }
MetricGroup::group_iterator MetricGroup::group_end() { return children_.end(); }
MetricGroup::const_group_iterator MetricGroup::group_cbegin() const { return children_.cbegin(); }
MetricGroup::const_group_iterator MetricGroup::group_cend() const { return children_.cend(); }

void MetricGroup::resolve_fallback(const std::string &key)
{
  // TODO
  auto it = attr_.find(key);
  if (it == attr_.end()) return;
  std::string &v = it->second;
  if (v == "_fallback")
  {
    R_ASSERT_M(true, "NotImplemented");
  }
}


// --------------------------- class OptionList

PrefData::PrefData() {}

bool PrefData::LoadOption(const std::string& path)
{
  OptionDesc option;
  std::string ext = GetExtension(path);
  if (ext != "xml") {
    Logger::Error("Error: Only xml option file can be read.");
    return false;
  }

  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
    return false;
  XMLElement* root = doc.RootElement();
  XMLElement* optnode;

  optnode = root->FirstChildElement("Option");
  while (optnode) {
    const char* name = optnode->Attribute("name");
    const char* constraint = optnode->Attribute("constraint");
    const char* desc = optnode->Attribute("desc");
    const char* def = optnode->Attribute("default");
    if (!name)
      continue;
    option.name = name;
    option.constraint = constraint ? constraint : "";
    option.desc = desc ? desc : "";
    option.def = def ? def : "";

    add_option(option);

    optnode = optnode->NextSiblingElement("Option");
  }

  return true;
}

bool PrefData::LoadOptionFromMetric(const MetricGroup& m)
{
  if (m.exist("Option")) {
    OptionDesc option;
    auto count = m.get<size_t>("Option");
    for (unsigned i = 0; i < count; ++i) {
      std::string option_str = m.get_str("Option" + std::to_string(i + 1));
      CommandArgs args(option_str, 2, true);
      option.name = args.Get_str(0);
      option.constraint = args.Get_str(1);
      add_option(option);
    }
  }
  return true;
}

bool PrefData::LoadValue(const std::string& path)
{
  // currently only supports xml type file.
  std::string ext = GetExtension(path);
  if (ext != "xml") {
    Logger::Error("Error: Only xml option file can be read.");
    return false;
  }

  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
    return false;
  XMLElement* root = doc.RootElement();
  XMLElement* node = root->FirstChildElement();
  while (node) {
    const char* val = node->Attribute("value");
    if (!val)
      continue;
    set(node->Name(), val);
    node = node->NextSiblingElement();
  }

  path_ = path;
  return true;
}

bool PrefData::SaveValue(const std::string& path)
{
  if (path.empty()) return false;
  tinyxml2::XMLDocument doc;
  XMLElement* root = doc.NewElement(kDefaultRootTagName);
  doc.InsertFirstChild(root);

  XMLElement* nodeval;
  for (auto& it : values_) {
    nodeval = doc.NewElement(it.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", it.second.c_str());
  }

  path_ = path;
  return doc.SaveFile(path.c_str()) == XML_SUCCESS;
}

bool PrefData::SaveValue()
{
  return SaveValue(path_);
}

void PrefData::Clear()
{
  options_.clear();
  values_.clear();
}

void PrefData::set(const std::string& key, const std::string& value)
{
  // XXX: delete if value is empty?
  values_[key] = value;
}

const std::string* PrefData::get(const std::string& key) const
{
  auto i = values_.find(key);
  if (i == values_.end()) return nullptr;
  else return &i->second;
}

std::string* PrefData::get(const std::string& key)
{
  auto i = values_.find(key);
  if (i == values_.end()) return nullptr;
  else return &i->second;
}

OptionDesc* PrefData::get_option(const std::string& key)
{
  for (unsigned i = 0; i < options_.size(); ++i)
    if (options_[i].name == key)
      return &options_[i];
  return nullptr;
}

void PrefData::add_option(const OptionDesc& option)
{
  if (option.name.empty()) return;
  for (unsigned i = 0; i < options_.size(); ++i) {
    if (options_[i].name == option.name) {
      options_[i] = option;
      return;
    }
  }
  options_.push_back(option);
  InitValueWithOption(option);
}

void PrefData::InitValueWithOption(const OptionDesc& option)
{
  // TODO: check is default value fits with constraint?
  // TODO: check constraint if value if already set before adding option?
  if (!option.def.empty())
    set(option.name, option.def);
}

// PrefValue class

void PrefValue<std::string>::_value_load()
{
  std::string* v = PREFDATA->get(name_);
  if (!v) return;
  v_ = *v;
}

void PrefValue<std::string>::_value_set()
{
  PREFDATA->set(name_, v_);
}

void PrefValue<int>::_value_load()
{
  std::string* v = PREFDATA->get(name_);
  if (!v) return;
  v_ = atoi(v->c_str());
}

void PrefValue<int>::_value_set()
{
  char _t[32];
  itoa(v_, _t, 10);
  PREFDATA->set(name_, _t);
}

void PrefValue<unsigned>::_value_load()
{
  std::string* v = PREFDATA->get(name_);
  if (!v) return;
  v_ = (unsigned) atoi(v->c_str());
}

void PrefValue<unsigned>::_value_set()
{
  char _t[32];
  itoa((int) v_, _t, 10);
  PREFDATA->set(name_, _t);
}

void PrefValue<bool>::_value_load()
{
  std::string *v = PREFDATA->get(name_);
  if (!v) return;
  v_ = (*v == "1") || (*v == "true");
}

void PrefValue<bool>::_value_set()
{
  PREFDATA->set(name_, v_ ? "true" : "false");
}


// PrefOptions class

PrefOptionList::PrefOptionList(const std::string& name, const std::string& values) :
  v_(name), index_(0), constraint_(values)
{
  Split(values, ',', list_);
  for (unsigned i = 0; i < list_.size(); ++i) {
    if (v_ == list_[i]) {
      index_ = i;
      return;
    }
  }
  set(0);
}

PrefOptionList::PrefOptionList(const std::string& name, const std::vector<std::string>& list) :
  v_(name), list_(list), index_(0)
{
  for (unsigned i = 0; i < list_.size(); ++i) {
    if (v_ == list_[i]) {
      index_ = i;
      return;
    }
  }
  set(0);
}

void PrefOptionList::set(const std::string& value)
{
  unsigned i = 0;
  if (list_.empty()) return;
  for (unsigned i = 0; i < list_.size(); ++i) {
    if (value == list_[i]) {
      v_ = value;
      index_ = i;
      break;
    }
  }
}

void PrefOptionList::set(unsigned i)
{
  if (list_.empty()) return;
  index_ = i < list_.size() ? i : 0;
  v_ = list_[index_];
}

const std::string& PrefOptionList::get() const
{
  return v_.get();
}

void PrefOptionList::next()
{
  if (list_.empty()) return;
  index_ = (index_ + 1) % list_.size();
  v_ = list_[index_];
}

void PrefOptionList::prev()
{
  if (list_.empty()) return;
  index_ = index_ == 0 ? list_.size() - 1 : index_ - 1;
  v_ = list_[index_];
}

unsigned PrefOptionList::index() const
{
  return index_;
}

const std::string& PrefOptionList::get_constraint() const
{
  return constraint_;
}

static std::vector<std::string> GetFileList(const std::string& path)
{
  std::vector<std::string> l;
  PATH->GetAllPaths(path, l);
  return l;
}

PrefOptionFile::PrefOptionFile(const std::string& name, const std::string& path) :
  PrefOptionList(name, GetFileList(path))
{
  constraint_ = path;
}

// ------------------------------------------------------------- Preference

/**
Preference::Preference() :
  resolution("1280x800",// "Screen resolution",
    "640x480,800x600,1280x960,1280x720,1440x900,1600x1050,1920x1200"),
  sound_buffer_size(2048,// "Sound buffer size (bigger, longer latency)",
    "512,1024,2048,3072,4096,8192"),
  gamemode(3, "Set gamemode to play.", "ALL,4Key,5Key,6Key,7Key,8Key,Popn,10Key,14Key", true),
  logging(1, "For development.", "0,1", true),
  maximum_thread(4, "Maximum working thread count that program will use", "0,1,2,3,4,6,8,12,16", true),
  soundset("", "Soundset file.", "sound/*.lr2ss", true),
  theme("themes/_fallback/theme.xml", "Set game theme file.", "themes/**.xml", true),
  theme_test("", "Test theme file (for debug purpose).", "", true),
  theme_select("", "Set game theme file for select scene.", "themes/**.lr2skin", true),
  theme_decide("", "Set game theme file for decide scene.", "themes/**.lr2skin", true),
  theme_play_7key("", "Set game theme file for 7key play.", "themes/**.lr2skin", true),
  theme_result("", "Set game theme file for result scene.", "themes/**.lr2skin", true),
  theme_courseresult("", "Set game theme file for course result scene.", "themes/**.lr2skin", true),
  theme_load_async(1, "Load theme more faster using multithread.", "0,1", true),
  select_sort_type(0, "Song sort type", "NOSORT,LEVEL,NAME,DIFFICULT,CLEARLAMP,CLEARRATE,JUDGERATE", true),
  select_difficulty_mode(0, "Song difficulty mode", "ALL,BEGINNER,NORMAL,HYPER,HARD,EXPERT,CHALLENGE", true)
{
  AddString("resolution", &resolution);
  AddInt("sound_buffer_size", &sound_buffer_size);
  AddInt("mode", &gamemode);
  AddInt("logging", &logging);
  AddInt("maxthreadcount", &maximum_thread);
  AddFile("theme", &theme);
  AddFile("TestScene", &theme_test);
  AddFile("SelectScene", &theme_select);
  AddFile("DecideScene", &theme_decide);
  AddFile("Play7Key", &theme_play_7key);
  AddFile("ResultScene", &theme_result);
  AddFile("CourseResultScene", &theme_courseresult);
  AddInt("themeloadasync", &theme_load_async);
  AddInt("sortmode", &select_sort_type);
  AddInt("difficulty", &select_difficulty_mode);
}

SystemPreference::~SystemPreference()
{
}
*/

// ------------------------------------------------------------------ KeyConfig

KeySetting::KeySetting() { Default(); }

void KeySetting::Default()
{
  memset(keycode_per_track_, 0, sizeof(keycode_per_track_));
  keycode_per_track_[0][0] = RI_KEY_LEFT_SHIFT;
  keycode_per_track_[1][0] = RI_KEY_Z;
  keycode_per_track_[2][0] = RI_KEY_S;
  keycode_per_track_[3][0] = RI_KEY_X;
  keycode_per_track_[4][0] = RI_KEY_D;
  keycode_per_track_[5][0] = RI_KEY_C;
  keycode_per_track_[6][0] = RI_KEY_F;
  keycode_per_track_[7][0] = RI_KEY_V;
  keycode_per_track_[8][0] = RI_KEY_M;
  keycode_per_track_[9][0] = RI_KEY_K;
  keycode_per_track_[10][0] = RI_KEY_COMMA;
  keycode_per_track_[11][0] = RI_KEY_L;
  keycode_per_track_[12][0] = RI_KEY_PERIOD;
  keycode_per_track_[13][0] = RI_KEY_SEMICOLON;
  keycode_per_track_[14][0] = RI_KEY_SLASH;
  keycode_per_track_[15][0] = RI_KEY_RIGHT_SHIFT;

  // TODO: gamepad
  // TODO: midi
}

void KeySetting::Load(const MetricGroup &metric)
{
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      metric.get_safe(format_string("Key%02u%02u", i, j), keycode_per_track_[i][j]);
}

void KeySetting::Load(const std::string& s)
{
  unsigned si = 0;
  char _t[5];
  _t[4] = '\0';

  // TODO: keymap per gamemode
  while (si < s.size() / 4) {
    memcpy(_t, s.c_str() + si * 4, 4);
    keycode_per_track_[si / 4][si % 4] = (unsigned)atoi(_t);
    ++si;
  }
}

void KeySetting::Save(MetricGroup &metric)
{
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      metric.set(format_string("Key%02u%02u", i, j), keycode_per_track_[i][j]);
}

void KeySetting::Save(std::string& s)
{
  std::stringstream ss;
  // TODO: keymap per gamemode
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      ss << format_string("%04u", keycode_per_track_[i][j]);
  s = ss.str();
}

int KeySetting::GetTrackFromKeycode(int keycode) const
{
  if (keycode == 0) return -1;
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      if (keycode_per_track_[i][j] == keycode) return i;
  return -1;
}



// ------------------------------ class Setting

/**
 * Initialize config objects.
 * @warn Must be called before initializing all other objects.
 */
void Setting::Initialize()
{
  METRIC = new MetricGroup();
  METRIC->set_name("game");
  PREFDATA = new PrefData();
  KEYPREF = new KeySetting();
  Setting::Reload();
}

/**
 * Cleanup config objects.
 * @warn Must be called after all other objects are destroyed.
 */
void Setting::Cleanup()
{
  delete KEYPREF;
  delete PREFDATA;
  delete METRIC;
  KEYPREF = nullptr;
  PREFDATA = nullptr;
  METRIC = nullptr;
}

void Setting::Reload()
{
  static const char* metric_option_attrs[] = {
    "SoundSet",
    "SelectScene",
    "DecideScene",
    "Play7Key",
    "ResultScene",
    "CourseResultScene",
    0
  };
  static const char* fallback_metric_path = "themes/_fallback/theme.xml";

  // clear non-static preference and clear theme metric.
  PREFDATA->Clear();
  KEYPREF->Default();
  METRIC->clear();

  // read preference value
  METRIC->Load(fallback_metric_path);
  PREFDATA->LoadValue(kSettingPath);
  auto* key = PREFDATA->get("key");
  if (key) KEYPREF->Load(*key);
}

void Setting::Save()
{
  std::string key;
  KEYPREF->Save(key);
  PREFDATA->set("key", key);
  if (!PREFDATA->SaveValue())
    Logger::Error("Failed to save system preference.");
}


PrefData* PREFDATA = nullptr;
KeySetting* KEYPREF = nullptr;
MetricGroup* METRIC = nullptr;

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

// ------------------------------------------------------------------ Loader/Helper

const char *_LR2name[] = {
  "SELECT",
  "DECIDE",
  "EXSELECT",
  "EXDECIDE",
  "FOLDER_OPEN",
  "FOLDER_CLOSE",
  "PANEL_OPEN",
  "PANEL_CLOSE",
  "OPTION_CHANGE",
  "DIFFICULTY",
  "SCREENSHOT",
  "CLEAR",
  "FAIL",
  "MINE",
  "SCRATCH",
  "COURSECLEAR",
  "COURSEFAIL",
  0
};

const char *_metricname[] = {
  "SongSelect",
  "SongDecide",
  "SongExSelect",
  "SongExDecide",
  0
};

class LR2CSVSoundHandlers
{
public:
  static bool set_sound_metric(LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    unsigned i = 0;
    MetricGroup *soundmetric = METRIC->get_group("Sound");
    R_ASSERT(soundmetric);
    while (_LR2name[i])
    {
      if (strcmp(ctx->get_str(0), _LR2name[i]) == 0)
      {
        soundmetric->set(_metricname[i], ctx->get_str(1));
        break;
      }
      ++i;
    }
    return true;
  }
  LR2CSVSoundHandlers()
  {
    unsigned i = 0;
    while (_LR2name[i])
    {
      LR2CSVExecutor::AddHandler(_LR2name[i], (LR2CSVHandlerFunc)&set_sound_metric);
      ++i;
    }
  }
};

// register handler
LR2CSVSoundHandlers _LR2CSVSoundHandlers;

}
