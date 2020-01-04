#include "Script.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "Setting.h"

/* Basic objects for CreateObjectFromMetric */
#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Line.h"
#include "object/Slider.h"
#include "object/Button.h"
#include "object/OnMouse.h"

#include "LR2/LR2SceneLoader.h"
#include "LR2/LR2Flag.h"
#include "Util.h"
#include <iostream>

namespace rhythmus
{

Script::Script() : scene_(nullptr)
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

void Script::SetContextScene(Scene *scene)
{
  scene_ = scene;
}

void Script::SetFlag(int flag_no, int value)
{
  flags_[flag_no] = value;
}

int Script::GetFlag(int flag_no) const
{
  if (flag_no >= 1000) return 0;
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

void Script::SetButtonNumber(size_t idx, int number)
{
  SetNumber(idx + 1000, number);
}

void Script::SetSliderNumber(size_t idx, int number)
{
  SetNumber(idx + 1500, number);
}

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
  else if (objtype == "Number")
    obj = new Number();
  else if (objtype == "Line")
    obj = new Line();
  else if (objtype == "Slider")
    obj = new Slider();
  else if (objtype == "Button")
    obj = new Button();
  else if (objtype == "OnMouse")
    obj = new OnMouse();

  if (!obj) return nullptr;
  obj->SetIgnoreVisibleGroup(false);
  obj->Load(metrics);

  return obj;
}

void Script::ExecuteLR2Script(const std::string &filepath)
{
#define NAMES \
  NAME("SRC_NOTE", "Note%d", "lr2src", 0), \
  NAME("SRC_AUTO_NOTE", "Note%dAuto", "lr2src", 0), \
  NAME("SRC_LN_END", "Note%dLNEnd", "lr2src", 0), \
  NAME("SRC_AUTO_LN_END", "Note%dAutoLNEndy", "lr2src", 0), \
  NAME("SRC_LN_START", "Note%dLNStart", "lr2src", 0), \
  NAME("SRC_AUTO_LN_START", "Note%dAutoLNStart", "lr2src", 0), \
  NAME("SRC_LN_BODY", "Note%dLNBody", "lr2src", 0), \
  NAME("SRC_AUTO_LN_BODY", "Note%dAutoLNBody", "lr2src", 0), \
  NAME("SRC_MINE", "Note%dMine", "lr2src", 0), \
  NAME("DST_NOTE", "NoteField", "Note%dlr2cmd", 0), \
  NAME("SRC_GROOVEGAUGE", "LifeGauge%dP", "lr2src", 0), \
  NAME("DST_GROOVEGAUGE", "LifeGauge%dP", "OnLR", 0), \
  NAME("SRC_NOWJUDGE_1P", "Judge1P", "lr2src", 0), \
  NAME("DST_NOWJUDGE_1P", "Judge1P", "OnLR", 0), \
  NAME("SRC_NOWJUDGE_2P", "Judge2P", "lr2src", 0), \
  NAME("DST_NOWJUDGE_2P", "Judge2P", "OnLR", 0), \
  NAME("SRC_BAR_BODY", "MusicWheelType%d", "lr2src", 0), \
  NAME("DST_BAR_BODY_ON", "MusicWheel", "BarOn%dOnLR", 0), \
  NAME("DST_BAR_BODY_OFF", "MusicWheel", "Bar%dOnLR", 0), \
  NAME("SRC_BAR_TITLE", "MusicWheelTitle%d", "lr2src", 0), \
  NAME("DST_BAR_TITLE", "MusicWheelTitle%d", "OnLR", 0), \
  NAME("SRC_BAR_LEVEL", "MusicWheelLevel%d", "lr2src", 0), \
  NAME("DST_BAR_LEVEL", "MusicWheelLevel%d", "OnLR", 0), \
  NAME("SRC_IMAGE", "Sprite", "lr2src", 2), \
  NAME("DST_IMAGE", "Sprite", "OnLR", 1), \
  NAME("SRC_TEXT", "Text", "lr2src", 2), \
  NAME("DST_TEXT", "Text", "OnLR", 1), \
  NAME("SRC_BARGRAPH", "Graph", "lr2src", 2), \
  NAME("DST_BARGRAPH", "Graph", "OnLR", 1), \
  NAME("SRC_SLIDER", "Slider", "lr2src", 2), \
  NAME("DST_SLIDER", "Slider", "OnLR", 1), \
  NAME("SRC_NUMBER", "Number", "lr2src", 2), \
  NAME("DST_NUMBER", "Number", "OnLR", 1), \
  NAME("SRC_JUDGELINE", "JudgeLine", "lr2src", 2), \
  NAME("DST_JUDGELINE", "JudgeLine", "OnLR", 1), \
  NAME("SRC_LINE", "Line", "lr2src", 2), \
  NAME("DST_LINE", "Line", "OnLR", 1), \
  NAME("SRC_ONMOUSE", "OnMouse", "lr2src", 2), \
  NAME("DST_ONMOUSE", "OnMouse", "OnLR", 1), \
  NAME("SRC_BUTTON", "Button", "lr2src", 2), \
  NAME("DST_BUTTON", "Button", "OnLR", 1)

#define NAME(lr2name, metricname, attr, _objtype) lr2name
  static const char *_lr2names[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr, _objtype) metricname
  static const char *_metricnames[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr, _objtype) attr
  static const char *_attrnames[] = {
    NAMES, 0
  };
#undef NAME

#define NAME(lr2name, metricname, attr, _objtype) _objtype
  /* 0:Metric 1:Use previous 2:Create */
  static const int _objtypes[] = {
    NAMES, 0
  };
#undef NAME

  LR2SceneLoader loader;
  std::vector<
    std::tuple<std::string, Metric*, bool>
    /* objtype, metric, is_ref */ >
    object_metrics;
  std::map<std::string, Metric*> object_processing;

  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);

  /*
   * flag initialization.
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
        std::string path = args.Get<std::string>(0);
        if (path != "CONTINUE")
          ResourceManager::SetAlias("Image" + std::to_string(image_idx++), path);
      }
      else if (lr2name == "LR2FONT")
      {
        std::string path = args.Get<std::string>(0);
        if (path != "CONTINUE")
          ResourceManager::SetAlias("Font" + std::to_string(font_idx++), path);
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
    int value_idx;
    size_t nameidx = 0;
    Split(lr2name, '_', obj_drawtype, obj_type);
    if (obj_type.empty())
      obj_drawtype.swap(obj_type);
    Split(v.second, ',', value_idx_str, value);
    value_idx = atoi(value_idx_str.c_str());
    while (_lr2names[nameidx])
    {
      if (_lr2names[nameidx] == lr2name)
        break;
      ++nameidx;
    }
    if (_metricnames[nameidx] == nullptr)
      continue;

    std::string name(format_string(_metricnames[nameidx], value_idx));
    std::string attrname(format_string(_attrnames[nameidx], value_idx));

    /* If not generic metrics(need to create one) and previously exists, update it.
     * If Thememetrics, retrieve one and update it.
     * Otherwise, create new metric (or retrive previous one). */
    Metric *curr_metrics = nullptr;
    if (_objtypes[nameidx] == 0)
    {
      if (object_processing[name])
      {
        curr_metrics = object_processing[name];
      }
      else
      {
        object_processing[name]
          = curr_metrics
          = Setting::GetThemeMetricList().create_metric(name);
        /* need to push metric to add zindex data */
        object_metrics.push_back({ name, curr_metrics, true });
      }
    }
    else if (_objtypes[nameidx] == 1)
    {
      curr_metrics = object_processing[name];
    }
    else
    {
      object_metrics.push_back({ name, new Metric(), false });
      object_processing[name]
        = curr_metrics
        = std::get<1>(object_metrics.back());
    }

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
      std::string loop(params[14]);

      /* parameter to 13th string */
      value.clear();
      for (size_t i = 0; i <= 13; ++i)
        value += params[i] + "|";
      value.pop_back();

      /* Retrieve previously generated event name.
       * if not, rename attrname using timer id */
      prev_value_exist = curr_metrics->exist("_timer" + attrname);
      if (prev_value_exist)
      {
        timer = curr_metrics->get<std::string>("_timer" + attrname);
        attrname = attrname + timer;
        value = curr_metrics->get<std::string>(attrname) + "," + value;
      }
      else
      {
        if (timer.empty())
          timer = "0";
        curr_metrics->set("_timer" + attrname, timer);
        attrname = attrname + timer;
        /* if first DST command, append timer / op code attribute. */
        value = "lr2cmd:" +
          params[16] + "|" + params[17] + "|" + params[18] + "|" + loop + "," +
          value;
        /* set Off event in first time */
        curr_metrics->set(attrname + "Off", "hide");
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
      if (obj_type == "TEXT" || obj_type == "BAR_TITLE")
        value = "Font" + value;
      else
        value = "Image" + value;
      /* Debug signal for op4 string. */
      CommandArgs args(value);
      if (args.size() > 12 && !args.Get<std::string>(12).empty())
        curr_metrics->set("_debug", args.Get<std::string>(12));
    }

    /* Set attribute */
    curr_metrics->set(attrname, value);
  }

  /* set zindex for all object metrics & process metric to scene */
  if (scene_)
  {
    size_t idx = 0;
    for (auto &m : object_metrics)
    {
      std::string &name = std::get<0>(m);
      Metric *metric = std::get<1>(m);
      bool is_delete = !std::get<2>(m);

      metric->set("zindex", (int)idx);
      BaseObject *obj = CreateObjectFromMetric(name, *metric);
      if (obj)
      {
        /* Without hide, all objects before timer activation is displayed.
         * So set all objects hidden before registering to children. */
        obj->Hide();
        scene_->RegisterChild(obj);
      }

      if (is_delete)
        delete metric;
#if 0
      else
      {
        if (obj = scene_->FindChildByName(name))
          obj->Load(metric);
        else
          std::cerr << "Warning: object metric '" << name <<
          "' is invalid and cannot appliable." << std::endl;
      }
#endif
      idx++;
    }
  }

  /* Afterwork: rename some metrics. */
  Setting::GetThemeMetricList().copy_metric(
    "MusicWheelTitle0", "MusicWheelTitle");

  /* Hook event */
  LR2Flag::HookEvent();
}

}