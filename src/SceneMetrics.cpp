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
ThemeMetrics::const_iterator ThemeMetrics::cbegin() { attr_.cbegin(); }
ThemeMetrics::iterator ThemeMetrics::end() { attr_.end(); }
ThemeMetrics::const_iterator ThemeMetrics::cend() { attr_.cend(); }

std::string GetCommandStringFromLR2SRC(const std::string &v)
{
  // TODO
  std::vector<std::string> params;
  MakeParamCountSafe(v, params, 13);
#if 0
  int attr = atoi(params[0].c_str());
  int imgidx = atoi(params[1].c_str());
  int sx = atoi(params[2].c_str());
  int sy = atoi(params[3].c_str());
  int sw = atoi(params[4].c_str());
  int sh = atoi(params[5].c_str());
  int divx = atoi(params[6].c_str());
  int divy = atoi(params[7].c_str());
  int cycle = atoi(params[8].c_str());    /* total loop time */
  int timer = atoi(params[9].c_str());    /* timer id in LR2 form */
#if 0
  int op1 = atoi_op(params[10].c_str());
  int op2 = atoi_op(params[11].c_str());
  int op3 = atoi_op(params[12].c_str());
#endif
#endif
  return "";
}

std::string GetCommandStringFromLR2DST(const std::string &v)
{
  // TODO
  std::vector<std::string> params;
  MakeParamCountSafe(v, params, 20);
#if 0
  int attr = atoi(params[0].c_str());
  int time = atoi(params[1].c_str());
  int x = atoi(params[2].c_str());
  int y = atoi(params[3].c_str());
  int w = atoi(params[4].c_str());
  int h = atoi(params[5].c_str());
  int acc_type = atoi(params[6].c_str());
  int a = atoi(params[7].c_str());
  int r = atoi(params[8].c_str());
  int g = atoi(params[9].c_str());
  int b = atoi(params[10].c_str());
  int blend = atoi(params[11].c_str());
  int filter = atoi(params[12].c_str());
  int angle = atoi(params[13].c_str());
  int center = atoi(params[14].c_str());
#if 0
  int loop = atoi(params[15].c_str());
  int timer = atoi(params[16].c_str());
  int op1 = atoi_op(params[17].c_str());
  int op2 = atoi_op(params[18].c_str());
  int op3 = atoi_op(params[19].c_str());
#endif
#endif
  return "";
}

void LoadMetricsLR2CSV(const std::string &filepath, Scene &scene)
{
  LR2SceneLoader loader;
  std::vector<ThemeMetrics> metric_list;
  std::map<std::string, ThemeMetrics*> metric_processing;

  //NAME("INFORMATION", "Information"), \ // -- information is ignored here
#define NAMES \
  NAME("CUSTOMOPTION", "CustomOption", "1234"), \
  NAME("CUSTOMFILE", "CustomFileOption", "1234"), \
  NAME("IMAGE", "CurrentSceneMetrics", "1234"), \
  NAME("FONT", "CurrentSceneMetrics", ""), \
  NAME("LR2FONT", "CurrentSceneMetrics", ""), \
  NAME("SRC_IMAGE", "Sprite", "Onload"), \
  NAME("DST_IMAGE", "Sprite", "1234"), \
  NAME("SRC_TEXT", "Text"), \
  NAME("DST_TEXT", "Text"), \
  NAME("SRC_BARGRAPH", "Graph"), \
  NAME("DST_BARGRAPH", "Graph"), \
  NAME("SRC_SLIDER", "Slider"), \
  NAME("DST_SLIDER", "Slider"), \
  NAME("SRC_NUMBER", "NumberSprite"), \
  NAME("DST_NUMBER", "NumberSprite"), \
  NAME("SRC_JUDGELINE", "JudgeLine"), \
  NAME("DST_JUDGELINE", "JudgeLine"), \
  NAME("SRC_LINE", "Line"), \
  NAME("DST_LINE", "Line"), \
  NAME("SRC_NOTE", "NoteField"), \
  NAME("SRC_AUTO_NOTE", "NoteField"), \
  NAME("SRC_LN_END", "NoteField"), \
  NAME("SRC_AUTO_LN_END", "NoteField"), \
  NAME("SRC_LN_START", "NoteField"), \
  NAME("SRC_AUTO_LN_START", "NoteField"), \
  NAME("SRC_LN_BODY", "NoteField"), \
  NAME("SRC_AUTO_LN_BODY", "NoteField"), \
  NAME("SRC_MINE", "NoteField"), \
  NAME("DST_NOTE", "NoteField"), \
  NAME("SRC_BUTTON", "Button"), \
  NAME("DST_BUTTON", "Button"), \
  NAME("SRC_GROOVEGAUGE", "LifeGauge"), \
  NAME("DST_GROOVEGAUGE", "LifeGauge"), \
  NAME("SRC_NOWJUDGE_1P", "Button"), \
  NAME("DST_NOWJUDGE_1P", "Button")

#define NAME(lr2name, metricname, attr) lr2name
  static const char *_lr2names[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr) metricname
  static const char *_metricnames[] = {
    NAMES, 0
  };
#undef NAME

  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

  // convert csv commands into metrics
  for (auto &v : loader)
  {
    if (v.first[0] != '#')
      continue;
    std::string lr2name = Upper(v.first.substr(1));
    std::string obj_type, obj_drawtype;
    size_t nameidx = 0;
    Split(lr2name, '_', obj_drawtype, obj_type);
    while (_lr2names[nameidx])
    {
      if (_lr2names[nameidx] == lr2name)
        break;
      ++nameidx;
    }
    if (_metricnames[nameidx] == nullptr)
      continue;
    std::string name = _metricnames[nameidx];
    std::string key = name + std::to_string(v.second[0]);
    std::vector<std::string> params;
    ThemeMetrics *curr_metrics = metric_processing[key];
    if (curr_metrics == nullptr || obj_drawtype == "SRC")
    {
      metric_list.emplace_back(ThemeMetrics(name));
      curr_metrics = metric_processing[key] = &metric_list.back();
    }

    if (obj_drawtype == "SRC")
    {
    }
    else if (obj_drawtype == "DST")
    {
    }
  }

  // TODO: load scene option from metric (default)

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