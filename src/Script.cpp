#include "Script.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Setting.h"

/* Basic objects for CreateObjectFromMetric */
#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"

#include "LR2/LR2SceneLoader.h"
#include "LR2/LR2Flag.h"
#include "Util.h"
#include <iostream>

namespace rhythmus
{

Script::Script()
{
  memset(flags_, 0, sizeof(flags_));
  memset(numbers_, 0, sizeof(numbers_));
  flags_[0] = 1;

  // XXX: by default ...? or in select scene start?
  flags_[50] = 1;  // OFFLINE
  flags_[52] = 1;  // EXTRA MODE OFF
}

Script::~Script()
{}

void Script::SetFlag(int flag_no, int value)
{
  flags_[flag_no] = value;
}

int Script::GetFlag(int flag_no) const
{
  if (flag_no < 0)
    return flags_[-flag_no] == 0 ? 1 : 0;
  else
    return flags_[flag_no] > 0 ? 1 : 0;
}

void Script::SetString(size_t idx, const std::string &value)
{
  strings_[idx] = value;
  EventManager::SendEvent("Text" + std::to_string(idx));
}

void Script::SetNumber(size_t idx, int number)
{
  numbers_[idx] = number;
  EventManager::SendEvent("Number" + std::to_string(idx));
}

const std::string& Script::GetString(size_t idx) const { return strings_[idx]; }
int Script::GetNumber(size_t idx) const { return numbers_[idx]; }

Script &Script::getInstance()
{
  static Script instance;
  return instance;
}

void Script::ExecuteFromPath(const std::string &path)
{
  std::string ext(GetExtension(path));
  if (ext == "lr2skin")
  {
    getInstance().ExecuteLR2Script(path);
  }
  else if (ext == "lua")
  {
    std::cerr << "Lua script is not supported currently." << std::endl;
    return;
  }
  else if (ext == "py")
  {
    std::cerr << "Python script is not supported currently." << std::endl;
    return;
  }
  else
  {
    std::cerr << "Unknown script: " << path << std::endl;
    return;
  }
}

constexpr char* kLR2SubstitutePath = "LR2files/Theme";
constexpr char* kSubstitutePath = "../themes";

BaseObject* CreateObjectFromMetric(const std::string &objtype, Metric &metrics)
{
  BaseObject *obj = nullptr;

  if (objtype == "Sprite")
    obj = new Sprite();
  else if (objtype == "Text")
    obj = new Text();
  else if (objtype == "NumberSprite")
    obj = new NumberSprite();

  if (!obj) return nullptr;
  obj->Load(metrics);

  return obj;
}

void Script::ExecuteLR2Script(const std::string &filepath)
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
  NAME("DST_NOTE", "NoteField", "lr2cmd", true), \
  NAME("SRC_GROOVEGAUGE", "LifeGauge", "sprite", true), \
  NAME("DST_GROOVEGAUGE", "LifeGauge", "lr2cmd", true), \
  NAME("SRC_NOWJUDGE_1P", "Judge1P", "sprite", true), \
  NAME("DST_NOWJUDGE_1P", "Judge1P", "lr2cmd", true), \
  NAME("SRC_NOWJUDGE_2P", "Judge2P", "sprite", true), \
  NAME("DST_NOWJUDGE_2P", "Judge2P", "lr2cmd", true), \
  NAME("SRC_BAR_BODY", "MusicWheel", "lr2src", true), \
  NAME("DST_BAR_BODY_ON", "MusicWheel", "lr2cmd", true), \
  NAME("DST_BAR_BODY_OFF", "MusicWheel", "lr2cmd", true), \
  NAME("SRC_BAR_TITLE", "MusicWheel", "BarTitle", true), \
  NAME("DST_BAR_TITLE", "MusicWheel", "BarTitlelr2cmd", true), \
  NAME("SRC_IMAGE", "Sprite", "sprite", false), \
  NAME("DST_IMAGE", "Sprite", "lr2cmd", false), \
  NAME("SRC_TEXT", "Text", "lr2font", false), \
  NAME("DST_TEXT", "Text", "lr2cmd", false), \
  NAME("SRC_BARGRAPH", "Graph", "sprite", false), \
  NAME("DST_BARGRAPH", "Graph", "lr2cmd", false), \
  NAME("SRC_SLIDER", "Slider", "sprite", false), \
  NAME("DST_SLIDER", "Slider", "lr2cmd", false), \
  NAME("SRC_NUMBER", "NumberSprite", "sprite", false), \
  NAME("DST_NUMBER", "NumberSprite", "lr2cmd", false), \
  NAME("SRC_JUDGELINE", "JudgeLine", "sprite", false), \
  NAME("DST_JUDGELINE", "JudgeLine", "lr2cmd", false), \
  NAME("SRC_LINE", "Line", "sprite", false), \
  NAME("DST_LINE", "Line", "lr2cmd", false), \
  NAME("SRC_BUTTON", "Button", "sprite", false), \
  NAME("DST_BUTTON", "Button", "lr2cmd", false)

#define NAME(lr2name, metricname, attr, is_metric) lr2name
  static const char *_lr2names[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr, is_metric) metricname
  static const char *_metricnames[] = {
    NAMES, 0
  };
#undef NAME

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

  Scene *scene = SceneManager::get_current_scene();
  LR2SceneLoader loader;
  std::vector<std::pair<std::string, Metric> /* objtype, metric */>
    object_metrics;
  std::map<std::string, Metric*> object_processing;

  ASSERT(scene);

  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);

  /*
   * initialization.
   */
  for (size_t i = 900; i < 1000; ++i)
    flags_[i] = 0;

  /*
   * load first time for initializing resource, flags.
   */
  {
    LR2SceneLoader loader;
    size_t image_idx = 0, font_idx = 0;
    loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
    loader.Load(filepath);
    for (auto &v : loader)
    {
      if (v.first[0] != '#')
        continue;
      std::string lr2name = Upper(v.first.substr(1));
      CommandArgs args(v.second);

      if (lr2name == "CUSTOMOPTION")
      {
        // load OPCODE from Option.
        std::string option_name(args.Get<std::string>(0));
        std::string option_val(Setting::GetThemeSetting().GetOption(option_name)->value<std::string>());
        int option_idx = atoi(option_val.c_str());
        int op_idx = args.Get<int>(1);
        flags_[op_idx + option_idx] = 1;
      }
      else if (lr2name == "CUSTOMFILE")
      {
        // make path priority from Option.
        std::string option_name(args.Get<std::string>(0));
        ResourceManager::getInstance().MakePathHigherPriority(
          Setting::GetThemeSetting().GetOption(option_name)->value<std::string>()
        );
      }
      else if (lr2name == "IMAGE")
      {
        // TODO: value with "continue" ?
        ResourceManager::SetAlias("Image" + std::to_string(image_idx++), args.Get<std::string>(0));
      }
      else if (lr2name == "LR2FONT")
      {
        // TODO: value with "continue" ?
        ResourceManager::SetAlias("Font" + std::to_string(font_idx++), args.Get<std::string>(0));
      }

    }
  }

  /*
   * load second time to create objects
   */
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

    if (lr2name == "SRC_BAR_BODY")
    {
      name = "BarType" + value_idx_str + name;
    }

    if (lr2name == "DST_BAR_BODY_ON" ||
      lr2name == "DST_BAR_BODY_OFF")
    {
      name = "Bar" + value_idx_str + name;
    }

    /* Create new metric (or retrive previous one)
     * If not special metric, then add new metric to metric_append_list. */
    Metric *curr_metrics = nullptr;
    if (object_processing[name] == nullptr || obj_drawtype == "SRC")
    {
      object_metrics.push_back({ name, Metric() });
      curr_metrics = &object_metrics.back().second;
    }
    else curr_metrics = object_processing[name];

    if (obj_drawtype == "DST")
    {
      /**
       * use lr2cmd attribute.
       *
       * format:
       * (timer),(op1),(op2),(op3),(loop);(commands)
       */

      std::string prev_value;
      std::vector<std::string> params;
      bool prev_value_exist;

      MakeParamCountSafe(value, params, 19);
      std::string timer(params[15]);

      /* parameter to 13th string */
      value = "lr2cmd:";
      for (size_t i = 0; i <= 13; ++i)
        value += params[i] + ",";
      value.pop_back();

      /* if first DST command, append timer / op code attribute. */
      prev_value_exist = curr_metrics->exist(attrname);
      if (prev_value_exist)
        value = curr_metrics->get<std::string>(attrname) + ";" + value;
      else
      {
        value = timer + "," +
          params[16] + "," + params[17] + "," + params[18] + "," + params[14] + ";" +
          value;
      }
    }
    else if (obj_drawtype == "SRC")
    {
      /**
       * use SRC attribute for Sprite, Text, etc.
       *
       * format:
       * (imgname),(sx),(sy),(sw),(sh),(divx),(divy),(cycle),(timer)
       * (fontname),(sourcename),(align),(editable)
       *
       * change filename alias.
       */
      if (obj_type == "TEXT")
        value = "Font" + value;
      else
        value = "Image" + value;
    }

    /* Set attribute */
    curr_metrics->set(attrname, value);
  }

  /* set zindex for all object metrics & process metric to scene */
  {
    size_t idx = 0;
    for (auto &m : object_metrics)
    {
      std::string &name = m.first;
      Metric &metric = m.second;

      metric.set("zindex", (int)idx);
      BaseObject *obj = CreateObjectFromMetric(name, metric);
      if (obj)
        scene->RegisterChild(obj);
      else
      {
        if (obj = scene->FindChildByName(name))
          obj->Load(metric);
        else
          std::cerr << "Warning: object metric '" << name <<
          "' is invalid and cannot appliable." << std::endl;
      }
      idx++;
    }
  }

  /* Hook event */
  LR2Flag::HookEvent();
}

}