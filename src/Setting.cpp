#include "Setting.h"
#include "Util.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "LR2/LR2SceneLoader.h"
#include "common.h"
#include <sstream>
#include "tinyxml2.h"

using namespace tinyxml2;

namespace rhythmus
{
  
constexpr const char* kSettingPath = "config/system.xml";
constexpr const char* kDefaultRootTagName = "setting";

#if USE_LR2_FEATURE
constexpr const char* kLR2SubstitutePath = "LR2files/Theme";
constexpr const char* kSubstitutePath = "themes";
#endif

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
  else if (ext == "LR2SKIN")
  {
    return LoadFromLR2Metric(path);
  }
  else if (ext == "LR2SS")
  {
    return LoadFromLR2SoundMetric(path);
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

#if USE_LR2_FEATURE == 1
struct LR2CommandDesc
{
  const char *name;
  const char *newname;
  bool not_create_if_exist;
  const char *attributes[64];
  const char *default_value[64];
};

static const LR2CommandDesc **GetLR2CommandDesc()
{
  static LR2CommandDesc information =
  { "INFORMATION", "", true, {"gamemode", "title", "artist", "previewimage", 0}, {0} };

  // TODO: more elegant format specifier?
  static LR2CommandDesc customoption =
  { "CUSTOMOPTION", "option", true,
    {"name", "id", "constraint:%3s,%4s,%5s,%6s,%7s,%8s,%9s,%10s", 0},
    {"type", "option", 0}
  };

  static LR2CommandDesc customfile =
  { "CUSTOMFILE", "option", true, {"name", "constraint", "default", 0}, {"type", "file", 0} };

  static LR2CommandDesc image =
  { "IMAGE", "path", false, {"path", "name:image%i", 0} };

  static LR2CommandDesc lr2font =
  { "LR2FONT", "path", false, {"path", "name:font%i", 0} };

  static LR2CommandDesc font =
  { "FONT", "FONT__", false, {"size", "thick", "type", "name", 0}, {0} };

  static LR2CommandDesc bar_center =
  { "BAR_CENTER", "", true, {"bar_center", 0}, {0} };

  static LR2CommandDesc bar_available =
  { "BAR_AVAILABLE", "", true, {"bar_available", 0}, {0} };

  static LR2CommandDesc transcolor =
  { "TRANSCLOLR", "", true, {"transcolor", 0}, {0} };

  static LR2CommandDesc startinput =
  { "STARTINPUT", "", true, {"startinput", 0}, {0} };

  static LR2CommandDesc ignoreinput =
  { "IGNOREINPUT", "", true, {"ignoreinput", 0}, {0} };

  static LR2CommandDesc fadeout =
  { "FADEOUT", "", true, {"fadeout", 0}, {0} };

  static LR2CommandDesc fadein =
  { "FADEIN", "", true, {"fadein", 0}, {0} };

  static LR2CommandDesc timeout =
  { "SCENETIME", "", true, {"timeout", 0}, {0} };

  static LR2CommandDesc loadstart =
  { "LOADSTART", "", true, {"loadstart", 0}, {0} };

  static LR2CommandDesc loadend =
  { "LOADEND", "", true, {"loadend", 0}, {0} };

  static LR2CommandDesc playstart =
  { "SCRATCHSIDE", "", true, {"playstart", 0}, {0} };

  static LR2CommandDesc helpfile =
  { "HELPFILE", "", true, {"help", 0}, {0} };

  static LR2CommandDesc src_sprite =
  { "SRC_IMAGE", "sprite", false,
  {"lr2src:%v", 0}, {"type", "sprite", 0} };

  static LR2CommandDesc dst_sprite =
  { "DST_IMAGE", "sprite", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_text =
  { "SRC_TEXT", "text", false,
  {"lr2src:%v", 0}, {"type", "text", 0} };

  static LR2CommandDesc dst_text =
  { "DST_TEXT", "text", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_number =
  { "SRC_NUMBER", "number", false,
  {"lr2src:%v", 0}, {"type", "number", 0} };

  static LR2CommandDesc dst_number =
  { "DST_NUMBER", "number", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_button =
  { "SRC_BUTTON", "button", false,
  {"lr2src:%v", 0}, {"type", "button", 0} };

  static LR2CommandDesc dst_button =
  { "DST_BUTTON", "button", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_onmouse =
  { "SRC_ONMOUSE", "onmouse", false,
  {"lr2src:%v", 0}, {"type", "button", 0} };

  static LR2CommandDesc dst_onmouse =
  { "DST_ONMOUSE", "onmouse", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_slider =
  { "SRC_SLIDER", "slider", false,
  {"lr2src:%v", 0}, {"type", "slider", 0} };

  static LR2CommandDesc dst_slider =
  { "DST_SLIDER", "slider", true /* WARN: must use last element */,
  {"lr2dst:%a%v", 0}, {0} };

  static LR2CommandDesc src_bar_body =
  { "SRC_BAR_BODY", "MusicWheel", false,
  {"lr2bar%0ssrc:%v", 0}, {0} };

  // TODO: bargraph
  // TODO: note -- stepmania
  //static LR2CommandDesc src_note =
  //{ "SRC_NOTE", "NoteField", false, { "!note%s...", 0 }, { 0 } };

  static const LR2CommandDesc *gLR2CommandDesc[] = {
    &information,
    &customoption,
    &customfile,
    &image,
    &lr2font,
    &font,
    &bar_center,
    &bar_available,
    &transcolor,
    &startinput,
    &ignoreinput,
    &fadeout,
    &fadein,
    &timeout,
    &loadstart,
    &loadend,
    &playstart,
    &helpfile,
    &src_sprite,
    &dst_sprite,
    &src_text,
    &dst_text,
    &src_number,
    &dst_number,
    &src_button,
    &dst_button,
    &src_onmouse,
    &dst_onmouse,
    &src_slider,
    &dst_slider,
    0
  };

  return gLR2CommandDesc;
}
#endif


/**
 * Load from LR2 metric file.
 * Each command (#XXX) converts into MetricGroup.
 *
 * TODO: make code tidy -- make pattern struct such as
 * (name), (newname), (not_create_if_exist), (attributes_per_comma)
 * IMAGE, image, false, _dummy, path, ...
 */
bool MetricGroup::LoadFromLR2Metric(const std::string &path)
{
#if USE_LR2_FEATURE == 1
  LR2SceneLoader loader;
  std::vector<std::string> params;
  size_t optioncount = 0, imagecount = 0, fontcount = 0;
  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(path);

  /* Set metric group using path
   * (kind of trick; no group information is given at file) */
  std::string group;
  if (path.find("select") != std::string::npos)
  {
    group = "SelectScene";
    auto &m = add_group("MusicWheel");
    m.set("CenterIndex", 10);
    m.set("PositionType", 1);  /* use bar position */
    m.set("BarCount", 20);     /* XXX: default 20? */
    /* below is filter setting for LR2. */
    m.set("GamemodeFilter", "none,IIDXSP,IIDXDP,popn");
    m.set("DifficultyFilter", "none,easy,normal,hard,ex,insane");
    m.set("SortType", "none,title,level,clear");
  }
  else if (path.find("decide") != std::string::npos)
  {
    group = "DecideScene";
  }
  else if (path.find("play") != std::string::npos)
  {
    group = "PlayScene";
    auto &m = add_group("NoteField");
    m.set("LR2TypeLane", true);
    /* XXX: move it to _default.xml later */
    auto &m1 = add_group("NoteField1P");
    m1.set("_fallback", "NoteField");
    auto &m2 = add_group("NoteField2P");
    m2.set("_fallback", "NoteField");
  }
  else if (path.find("courseresult") != std::string::npos) /* must courseresult first */
  {
    group = "CourseResultScene";
  }
  else if (path.find("result") != std::string::npos)
  {
    group = "ResultScene";
  }

  if (group.empty())
  {
    Logger::Error("Unknown type of LR2 metric file: %s", path.c_str());
    return false;
  }

  // default scene settings
  set("usecustomlayout", true);
  //set("sort", true);


  std::string name;                         // command name
  std::string value;                        // command value
  const char *value_ptr;                    // pointer to current value
  std::string attr_name;                    // current attribute name
  std::string attr_value;                   // current attribute value. (only used if need to modified)
  std::string value_reg[128];               // value register (maximum 128)
  int value_reg_index;                      // current value register number
  std::map<std::string, int> command_count; // how many times command was called

  // convert csv commands into metrics
  for (auto &v : loader)
  {
    if (v.first[0] != '#')
      continue;

    /* We're going to load resource information into metrics,
     * So ignore #ENDOFHEADER sign to read them all. */
     //if (v.first == "#ENDOFHEADER")
     //  break;

    /* set command name and its value. */
    value_reg[0] = name = Upper(v.first.substr(1));
    value = v.second;
    value_ptr = value.c_str();
    value_reg_index = 1;
    command_count[name]++;

    /* process LR2 command */
    const auto *lr2desc = GetLR2CommandDesc();
    while (*lr2desc)
    {
      if (strcmp(name.c_str(), (*lr2desc)->name) == 0)
      {
        // set which metric to write
        MetricGroup *m = nullptr;
        if ((*lr2desc)->newname == nullptr || *(*lr2desc)->newname == 0)
        {
          m = this;
        }
        else
        {
          if ((*lr2desc)->not_create_if_exist)
            m = get_group((*lr2desc)->newname);
          if (!m)
            m = &add_group((*lr2desc)->newname);
        }
        // parse value and add it to value register
        for (unsigned i = 0; i < 128; ++i) value_reg[i].clear();
        while (*value_ptr && value_reg_index < 128)
        {
          while (*value_ptr != ',')
          {
            value_reg[value_reg_index].push_back(*value_ptr);
            ++value_ptr;
          }
          ++value_ptr;
          ++value_reg_index;
        }
        // set attribute name / value.
        const char *const *attrname = (*lr2desc)->attributes;
        value_reg_index = 1;
        while (*attrname)
        {
          const char *attr_ptr = *attrname;
          std::string *string_to_fill = &attr_name;
          attr_name.clear();
          attr_value.clear();
          while (*attr_ptr && *attr_ptr != ',')
          {
            if (*attr_ptr == ':')
            {
              // fill attr_value
              string_to_fill = &attr_value;
              ++attr_ptr;
              continue;
            }
            else if (*attr_ptr == '%')
            {
              // resolve format specifier
              int reg_idx = 0;
              char escapecmd = 0;
              ++attr_ptr;
              while (*attr_ptr)
              {
                if (*attr_ptr >= '0' && *attr_ptr <= '9')
                {
                  reg_idx = reg_idx * 10 + (*attr_ptr - '0');
                  ++attr_ptr;
                }
                else
                {
                  escapecmd = *attr_ptr;
                  ++attr_ptr;
                  break;
                }
              }
              switch (escapecmd)
              {
              case 'd':
                *string_to_fill += std::to_string(atoi(value_reg[reg_idx].c_str()));
                break;
              case 's':
                *string_to_fill += value_reg[reg_idx];
                break;
              case 'i':
                // get command count(index)
                // \comment start index by zero (subtract 1)
                *string_to_fill += std::to_string(command_count[name] - 1);
                break;
              case 'a':
                // get whole string of previous attribute
                // (do nothing if attribute not exists)
                if (m->exist(attr_name))
                  *string_to_fill += m->get_str(attr_name) + "|";
                break;
              case 'v':
                // get whole original value
                *string_to_fill += value;
                break;
              case 0:
                break;
              default:
                (*string_to_fill).push_back(escapecmd);
              }
            }
            else
            {
              (*string_to_fill).push_back(*attr_ptr);
              ++attr_ptr;
            }
          }
          // if empty attrname, then don't add it to value (silent)
          if (*attrname)
          {
            m->set(attr_name, attr_value.empty()
              ? value_reg[value_reg_index]
              : attr_value);
          }
          ++attrname;
          ++value_reg_index;
        }
      }
      ++lr2desc;
    }
  }

  /* turn on path prefix for LR2. */
  PATH->SetPrefixReplace(
    "LR2files/Theme", kSubstitutePath
  );
  return true;
#else
  return false;
#endif
}

bool MetricGroup::LoadFromLR2SoundMetric(const std::string &path)
{
#if USE_LR2_FEATURE == 1
  LR2SceneLoader loader;
  loader.SetSubStitutePath("LR2files", "");
  loader.Load(path);

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

  for (auto &v : loader)
  {
    size_t metric_idx = 0;
    std::string lr2name;
    std::string metricname;
    if (v.first[0] != '#')
      continue;
    lr2name = v.first.substr(1);

    std::string vv = Substitute(
      GetFirstParam(v.second), "LR2files", ""
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
    set(metricname, vv);
  }
  return true;
#else
  return false;
#endif
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


// ----------------------------- class Option

Option::Option()
  : type_("option"), curr_sel_idx_(0),
    save_with_constraint_(false), visible_(false) {}

template <> const std::string& Option::value() const
{ return value_; }
template <> std::string Option::value() const
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
{ return (float)atof(value_.c_str()); }
template <> bool Option::value() const
{ return strnicmp(value_.c_str(), "true", 4) == 0
      || strnicmp(value_.c_str(), "on", 2) == 0; }

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
  default_.clear();
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
  // set default if it's empty.
  if (default_.empty() && !options_.empty())
    default_ = options_.front();
}

void Option::set_value(const std::string& option)
{
  auto i = std::find(options_.begin(), options_.end(), option);
  if (i == options_.end())
    value_ = default_;
  else
    value_ = option;
}

void Option::set_value(int value)
{
  set_value(std::to_string(value));
}

Option &Option::reset_value()
{
  set_value(default_);
  return *this;
}

void Option::save_with_constraint(bool v)
{
  save_with_constraint_ = v;
}

Option* Option::CreateOption(const std::string &optionstr)
{
  Option *option = nullptr;
  std::string optionvalue;
  if (optionstr.size() >= 2 && optionstr[0] == '!')
  {
    if (optionstr[1] == 'F')
    {
      // create as file option
      option = new FileOption();
      optionvalue = optionstr.substr(2);
    }
    else if (optionstr[1] == 'N')
    {
      // create as number option
      option = new NumberOption();
      optionvalue = optionstr.substr(2);
    }
    else if (optionstr[1] == 'T')
    {
      // create as number option & return it directly
      option = new TextOption();
      option->set_value(optionstr.substr(2));
      return option;
    }
  }
  else
  {
    option = new Option();
    optionvalue = optionstr;
  }

  if (option)
    option->SetOption(optionvalue);

  return option;
}

Option &Option::show() { visible_ = true; return *this; }
Option &Option::hide() { visible_ = false; return *this; }

FileOption::FileOption() {}

void FileOption::SetOption(const std::string &options)
{
  PATH->GetAllPaths(options, options_);
  // set default if it's empty.
  if (default_.empty() && !options_.empty())
    default_ = options_.front();
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
    return false;
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

void OptionList::LoadFromMetrics(const MetricGroup &metric)
{
  if (!metric.exist("Option"))
    return;
  int optioncount = metric.get<int>("Option");
  for (int i = 0; i < optioncount; ++i)
  {
    /* attr,default,select list */
    CommandArgs args(metric.get_str("Option" + std::to_string(i)), 3, true);
    auto &option = SetOption(args.Get_str(0), args.Get_str(2));
    option.SetDefault(args.Get_str(1));
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
  auto it = options_.find(key);
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

void OptionList::AddOptionFromMetric(MetricGroup *metric)
{
  if (!metric) return;
  if (metric->exist("Option"))
  {
    auto count = metric->get<size_t>("Option");
    for (unsigned i = 0; i < count; ++i)
    {
      std::string option_str = metric->get_str("Option" + std::to_string(i+1));
      CommandArgs args(option_str, 2, true);
      SetOption(args.Get_str(0), args.Get_str(0));
    }
  }
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


// ------------------------------ class Setting

void Setting::Initialize()
{
  PREFERENCE = new SystemPreference();
  METRIC = new MetricGroup();
  METRIC->set_name("game");
}

void Setting::Cleanup()
{
  delete PREFERENCE;
  delete METRIC;
  PREFERENCE = nullptr;
  METRIC = nullptr;
}

void Setting::Reload()
{
  static const char *metric_option_attrs[] = {
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
  PREFERENCE->Clear();
  METRIC->clear();

  // read metrics --> create non-static preference
  METRIC->Load(fallback_metric_path);
  {
    std::string filepath;
    for (size_t i = 0; metric_option_attrs[i]; ++i)
    {
      auto *pref = PREFERENCE->GetFile(metric_option_attrs[i]);
      if (!pref || (**pref).empty()) continue;

      MetricGroup m;
      if (m.Load(**pref))
        PREFERENCE->Load(m);
    }
  }

  // read preference value
  PREFERENCE->Load(kSettingPath);
}

void Setting::Save()
{
  PREFERENCE->Save(kSettingPath);
}


// ------------------------------------------------------------- PreferenceList

PreferenceList::~PreferenceList() { Clear(); }

void PreferenceList::AddInt(const std::string &key, Preference<int> *pf)
{
  auto i = pref_i_.find(key);
  R_ASSERT(i == pref_i_.end());
  pref_i_[key] = pf;
  if (!pf->is_static())
    non_static_prefs_.push_back(pf);
}

void PreferenceList::AddFloat(const std::string &key, Preference<float> *pf)
{
  auto i = pref_f_.find(key);
  R_ASSERT(i == pref_f_.end());
  pref_f_[key] = pf;
  if (!pf->is_static())
    non_static_prefs_.push_back(pf);
}

void PreferenceList::AddString(const std::string &key,
  Preference<std::string> *pf)
{
  auto i = pref_s_.find(key);
  R_ASSERT(i == pref_s_.end());
  pref_s_[key] = pf;
  if (!pf->is_static())
    non_static_prefs_.push_back(pf);
}

void PreferenceList::AddFile(const std::string &key,
  Preference<std::string> *pf)
{
  auto i = pref_file_.find(key);
  R_ASSERT(i == pref_file_.end());
  pref_file_[key] = pf;
  if (!pf->is_static())
    non_static_prefs_.push_back(pf);
}

void PreferenceList::SetInt(const std::string &key, int v)
{
  auto *pf = GetInt(key);
  if (pf) *pf = v;
}

void PreferenceList::SetFloat(const std::string &key, float v)
{
  auto *pf = GetFloat(key);
  if (pf) *pf = v;
}

void PreferenceList::SetString(const std::string &key, const std::string &v)
{
  auto *pf = GetString(key);
  if (pf) *pf = v;
}

void PreferenceList::SetFile(const std::string &key, const std::string &v)
{
  auto *pf = GetFile(key);
  if (pf) *pf = v;
}

Preference<int>* PreferenceList::GetInt(const std::string &key)
{
  auto i = pref_i_.find(key);
  if (i != pref_i_.end()) return i->second;
  else return nullptr;
}

Preference<float>* PreferenceList::GetFloat(const std::string &key)
{
  auto i = pref_f_.find(key);
  if (i != pref_f_.end()) return i->second;
  else return nullptr;
}

Preference<std::string>* PreferenceList::GetString(const std::string &key)
{
  auto i = pref_s_.find(key);
  if (i != pref_s_.end()) return i->second;
  else return nullptr;
}

Preference<std::string>* PreferenceList::GetFile(const std::string &key)
{
  auto i = pref_file_.find(key);
  if (i != pref_file_.end()) return i->second;
  else return nullptr;
}

void PreferenceList::Load(const std::string &path)
{
  MetricGroup m;
  m.Load(path);
  Load(m);
}

void PreferenceList::Load(const MetricGroup &m)
{
  // Add elements if necessary
  for (auto i = m.group_cbegin(); i != m.group_cend(); ++i)
  {
    if (i->name() == "option")
    {
      std::string name;
      std::string type;
      std::string constraint;
      std::string desc = "(none)";
      i->get_safe("name", name);
      i->get_safe("type", type);
      i->get_safe("con", constraint);
      i->get_safe("desc", desc);
      if (name.empty() || type.empty()) continue;
      if (type == "int" || type == "number")
      {
        int def;
        i->get_safe("def", def);
        pref_i_[name] = new Preference<int>(def, desc, constraint, false);
      }
      else if (type == "float")
      {
        double def;
        i->get_safe("def", def);
        pref_f_[name] = new Preference<float>((float)def, desc, constraint, false);
      }
      else if (type == "text" || type == "string" || type == "option"
            || type == "file")
      {
        std::string def;
        i->get_safe("def", def);
        auto *pref = new Preference<std::string>(def, desc, constraint, false);
        if (type == "file")
          pref_file_[name] = pref;
        else
          pref_s_[name] = pref;
      }
    }
  }

  // Load values
  for (auto i : pref_i_)
    m.get_safe(format_string("%s.value", i.first.c_str()), **i.second);
  for (auto i : pref_f_)
    m.get_safe(format_string("%s.value", i.first.c_str()), **i.second);
  for (auto i : pref_s_)
    m.get_safe(format_string("%s.value", i.first.c_str()), **i.second);
  for (auto i : pref_file_)
    m.get_safe(format_string("%s.value", i.first.c_str()), **i.second);
}

bool PreferenceList::Save(const std::string &path)
{
  if (path.empty()) return false;
  tinyxml2::XMLDocument doc;
  XMLElement *root = doc.NewElement(kDefaultRootTagName);
  doc.InsertFirstChild(root);

  XMLElement *nodeval;

  for (auto &i : pref_i_)
  {
    nodeval = doc.NewElement(i.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", **i.second);
  }
  for (auto &i : pref_f_)
  {
    nodeval = doc.NewElement(i.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", **i.second);
  }
  for (auto &i : pref_s_)
  {
    nodeval = doc.NewElement(i.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", (**i.second).c_str());
  }
  for (auto &i : pref_file_)
  {
    nodeval = doc.NewElement(i.first.c_str());
    root->InsertFirstChild(nodeval);
    nodeval->SetAttribute("value", (**i.second).c_str());
  }

  return doc.SaveFile(path.c_str()) == XML_SUCCESS;
}

void PreferenceList::Clear()
{
  // Non-static preference objects should be stored separated.
  // If not, static preference memory will be removed first,
  // And Clear() method will be work after that. And at that time
  // we cannot see preference->is_static? member correctly,
  // which will see wrong memory address and cause SIGSEGV.
  for (auto *p : non_static_prefs_)
    delete p;
  non_static_prefs_.clear();

  // XXX: tidy code with template
  for (auto i = pref_i_.begin(); i != pref_i_.end(); )
  {
    if (!i->second->is_static())
      pref_i_.erase(i++);
    else
      ++i;
  }

  for (auto i = pref_f_.begin(); i != pref_f_.end(); )
  {
    if (!i->second->is_static())
      pref_f_.erase(i++);
    else
      ++i;
  }

  for (auto i = pref_s_.begin(); i != pref_s_.end(); )
  {
    if (!i->second->is_static())
      pref_s_.erase(i++);
    else
      ++i;
  }

  for (auto i = pref_file_.begin(); i != pref_file_.end(); )
  {
    if (!i->second->is_static())
      pref_file_.erase(i++);
    else
      ++i;
  }
}

SystemPreference::SystemPreference() :
  resolution("1280x800", "Screen resolution",
    "640x480,800x600,1280x960,1280x720,1440x900,1600x1050,1920x1200", true),
  sound_buffer_size(2048, "Sound buffer size (bigger, longer latency)",
    "512,1024,2048,3072,4096,8192", true),
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

void KeySetting::Save(MetricGroup &metric)
{
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      metric.set(format_string("Key%02u%02u", i, j), keycode_per_track_[i][j]);
}

int KeySetting::GetTrackFromKeycode(int keycode) const
{
  if (keycode == 0) return -1;
  for (unsigned i = 0; i < kMaxLaneCount; ++i)
    for (unsigned j = 0; j < 4; ++j)
      if (keycode_per_track_[i][j] == keycode) return i;
  return -1;
}

MetricGroup *METRIC = nullptr;
SystemPreference *PREFERENCE = nullptr;

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
