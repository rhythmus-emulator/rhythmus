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

void ThemeMetrics::set(const std::string &key, const std::string &v)
{
  attr_[key] = v;
}

void ThemeMetrics::set(const std::string &key, int v)
{
  set(key, std::to_string(v));
}

void ThemeMetrics::set(const std::string &key, double v)
{
  set(key, std::to_string(v));
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
ThemeMetrics::const_iterator ThemeMetrics::cbegin() const { attr_.cbegin(); }
ThemeMetrics::iterator ThemeMetrics::end() { attr_.end(); }
ThemeMetrics::const_iterator ThemeMetrics::cend() const { attr_.cend(); }

void LoadMetricsLR2CSV(const std::string &filepath, Scene &scene)
{
  LR2SceneLoader loader;
  std::vector<std::string> params;
  size_t optioncount = 0, imagecount = 0, fontcount = 0;
  ThemeMetrics *curr_metrics = SceneManager::createMetrics(scene.get_name());
  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

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
    }
    else if (lr2name == "CUSTOMOPTION")
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

void LoadMetricsLR2SS(const std::string &filepath, Scene &scene)
{
  LR2SceneLoader loader;
  loader.SetSubStitutePath("LR2files", "..");
  loader.Load(filepath);

#define NAMES \
  NAME("SELECT", "SoundSongSelect"), \
  NAME("DECIDE", "SoundSongDecide"), \
  NAME("EXSELECT", "SoundSongExSelect"), \
  NAME("EXDECIDE", "SoundSongExDecide"), \
  NAME("FOLDER_OPEN", "SoundSongFolderOpen"), \
  NAME("FOLDER_CLOSE", "SoundSongFolderClose"), \
  NAME("PANEL_OPEN", "SoundPanelOpen"), \
  NAME("PANEL_CLOSE", "SoundPanelClose"), \
  NAME("OPTION_CHANGE", "SoundChangeOption"), \
  NAME("DIFFICULTY", "SoundChangeDifficulty"), \
  NAME("SCREENSHOT", "SoundScreenshot"), \
  NAME("CLEAR", "SoundClear"), \
  NAME("FAIL", "SoundFail"), \
  NAME("MINE", "SoundMineNote"), \
  NAME("SCRATCH", "SoundSongSelectChange"), \
  NAME("COURSECLEAR", "SoundCourseClear"), \
  NAME("COURSEFAIL", "SoundCourseFail")

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
    SceneManager::createMetrics(metricname)->set("path", vv);
  }
}

void LoadMetrics(const std::string &filepath)
{
  std::string ext = Upper(GetExtension(filepath));
  Scene *scene = SceneManager::get_current_scene();

  if (ext == "INI")
  {
    // TODO: Stepmania metrics info
    std::cerr << "Warning: Stepmania skin is not supported yet" << std::endl;
  }
  else if (ext == "LR2SKIN")
  {
    LoadMetricsLR2CSV(filepath, *scene);
  }
  else if (ext == "LR2SS")
  {
    LoadMetricsLR2SS(filepath, *scene);
  }
  else
  {
    std::cerr << "Error: not supported ThemeMetrics file: "
      << filepath << std::endl;
  }
}

void LoadScriptLR2CSV(const std::string &filepath, Scene &scene)
{
#define NAMES \
  NAME("SRC_NOTE", "NoteField", "NoteSprite", true), \
  NAME("SRC_AUTO_NOTE", "NoteField", "AutoNoteSprite", true), \
  NAME("SRC_LN_END", "NoteField", "LNEndSprite", true), \
  NAME("SRC_AUTO_LN_END", "NoteField", "AutoLNEndSprite", true), \
  NAME("SRC_LN_START", "NoteField", "LNBeginSprite", true), \
  NAME("SRC_AUTO_LN_START", "NoteField", "AutoLNBeginSprite", true), \
  NAME("SRC_LN_BODY", "NoteField", "LNBodySprite", true), \
  NAME("SRC_AUTO_LN_BODY", "NoteField", "AutoLNBodySprite", true), \
  NAME("SRC_MINE", "NoteField", "MineSprite", true), \
  NAME("DST_NOTE", "NoteField", "LR2cmdinit", true), \
  NAME("SRC_GROOVEGAUGE", "LifeGauge", "Sprite", true), \
  NAME("DST_GROOVEGAUGE", "LifeGauge", "LR2cmdinit", true), \
  NAME("SRC_NOWJUDGE_1P", "Judge1P", "Sprite", true), \
  NAME("DST_NOWJUDGE_1P", "Judge1P", "LR2cmdinit", true), \
  NAME("SRC_NOWJUDGE_2P", "Judge2P", "Sprite", true), \
  NAME("DST_NOWJUDGE_2P", "Judge2P", "LR2cmdinit", true), \
  NAME("SRC_IMAGE", "Sprite", "Sprite", false), \
  NAME("DST_IMAGE", "Sprite", "LR2cmdinit", false), \
  NAME("SRC_TEXT", "Text", "Font", false), \
  NAME("DST_TEXT", "Text", "LR2cmdinit", false), \
  NAME("SRC_BARGRAPH", "Graph", "Sprite", false), \
  NAME("DST_BARGRAPH", "Graph", "LR2cmdinit", false), \
  NAME("SRC_SLIDER", "Slider", "Sprite", false), \
  NAME("DST_SLIDER", "Slider", "LR2cmdinit", false), \
  NAME("SRC_NUMBER", "NumberSprite", "Sprite", false), \
  NAME("DST_NUMBER", "NumberSprite", "LR2cmdinit", false), \
  NAME("SRC_JUDGELINE", "JudgeLine", "Sprite", false), \
  NAME("DST_JUDGELINE", "JudgeLine", "LR2cmdinit", false), \
  NAME("SRC_LINE", "Line", "Sprite", false), \
  NAME("DST_LINE", "Line", "LR2cmdinit", false), \
  NAME("SRC_BUTTON", "Button", "Sprite", false), \
  NAME("DST_BUTTON", "Button", "LR2cmdinit", false)

#define NAME(lr2name, metricname, attr, is_metric) lr2name
  static const char *_lr2names[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr, is_metric) metricname
  static const char *_metricnames[] = {
    NAMES, 0
  };

#define NAME(lr2name, metricname, attr, is_metric) attr
  static const char *_attrnames[] = {
    NAMES, 0
  };
#undef NAME

#if 0
#define NAME(lr2name, metricname, attr, is_metric) is_metric
  static const bool _is_metric[] = {
    NAMES, 0
  };
#undef NAME
#endif

  LR2SceneLoader loader;
  std::vector<ThemeMetrics> object_metrics;
  std::map<std::string, ThemeMetrics*> object_processing;

  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

  // convert csv commands into metrics
  for (auto &v : loader)
  {
    if (v.first[0] != '#')
      continue;

    /* Get metric name from LR2 command */
    std::string lr2name = Upper(v.first.substr(1));
    std::string obj_type, obj_drawtype;
    std::string value_idx_str, value;
    size_t nameidx = 0;
    Split(lr2name, '_', obj_drawtype, obj_type);
    if (obj_type.empty())
      obj_drawtype.swap(obj_type);
    Split(v.second, ',', value_idx_str, value);
    while (_lr2names[nameidx])
    {
      if (_lr2names[nameidx] == lr2name)
        break;
      ++nameidx;
    }
    if (_metricnames[nameidx] == nullptr)
      continue;

    std::string name(_metricnames[nameidx]);
    std::string attrname(_attrnames[nameidx]);

    if (lr2name == "DST_NOTE" ||
      lr2name == "SRC_NOTE" ||
      lr2name == "SRC_NOTE" ||
      lr2name == "SRC_LN_END" ||
      lr2name == "SRC_AUTO_LN_END" ||
      lr2name == "SRC_LN_START" ||
      lr2name == "SRC_AUTO_LN_START" ||
      lr2name == "SRC_LN_BODY" ||
      lr2name == "SRC_AUTO_LN_BODY")
    {
      // add number to object name
      // e.g. OnLoad --> Note1OnLoad
      name = "Note" + value_idx_str + name;
    }

    /* Create new metric (or retrive previous one)
     * If not special metric, then add new metric to metric_append_list. */
    ThemeMetrics *curr_metrics = nullptr;
    if (object_processing[name] == nullptr || obj_type == "SRC")
    {
      object_metrics.emplace_back(ThemeMetrics(name));
      curr_metrics = &object_metrics.back();
    }
    else curr_metrics = object_processing[name];

    if (obj_drawtype == "DST")
    {
      /* use LR2cmdinit attribute. */
      std::string prev_value;
      std::vector<std::string> params;
      bool prev_value_exist;

      prev_value_exist = curr_metrics->get(attrname, prev_value);
      MakeParamCountSafe(value, params, 20);
      std::string timer(params[16]);

      if (prev_value_exist)
        value = prev_value + "|" + value;
    }
    else if (obj_drawtype == "SRC")
    {
      /* use Sprite attribute. */
      /* (nothing to do currently) */
    }

    /* Set attribute */
    curr_metrics->set(attrname, value);
  }

  /* set zindex for all object metrics & process metric to scene */
  {
    size_t idx = 0;
    for (auto &metric : object_metrics)
    {
      metric.set("zindex", (int)idx);
      scene.LoadObjectMetrics(metric);
      idx++;
    }
  }
}

void LoadScript(const std::string &filepath)
{
  std::string ext = Upper(GetExtension(filepath));
  Scene *scene = SceneManager::get_current_scene();

  if (ext == "XML")
  {
    // TODO: ?
    std::cerr << "Warning: Xml type skin is not supported yet" << std::endl;
  }
  else if (ext == "LUA")
  {
    // TODO
    std::cerr << "Warning: Lua script is not supported yet" << std::endl;
  }
  else if (ext == "LR2SKIN")
  {
    LoadScriptLR2CSV(filepath, *scene);
  }
  else
  {
    std::cerr << "Error: not supported scene script file: "
      << filepath << std::endl;
  }
}

/* Internally called by Scene::LoadMetric() */
BaseObject* CreateObjectFromMetrics(const ThemeMetrics &metrics)
{
  BaseObject *obj = nullptr;
  std::string _typename = metrics.name();
  metrics.get("type", _typename);

  if (_typename == "Sprite")
    obj = new Sprite();
  else if (_typename == "Text")
    obj = new Text();

  if (!obj) return nullptr;
  obj->Load(metrics);

  return obj;
}

}