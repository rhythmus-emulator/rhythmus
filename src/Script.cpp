#include "Script.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Setting.h"
#include "KeyPool.h"
#include "tinyxml2.h"
#include <fstream>

#include "Util.h"
#include "common.h"

namespace rhythmus
{

using namespace tinyxml2;

// ----------------------------------------------------------------- XMLContext

XMLContext::XMLContext() : ctx(nullptr), curr_node(nullptr)
{
  ctx = new tinyxml2::XMLDocument();
  curr_node = nullptr;
}

XMLContext::XMLContext(const std::string &rootnodename) : ctx(nullptr), curr_node(nullptr)
{
  ctx = new tinyxml2::XMLDocument();
  auto* doc = (tinyxml2::XMLDocument*)ctx;
  curr_node = doc->InsertFirstChild(doc->NewElement(rootnodename.c_str()));
}

XMLContext::~XMLContext()
{
  delete (tinyxml2::XMLDocument*)ctx;
}

bool XMLContext::Load(const std::string &path)
{
  auto* doc = (tinyxml2::XMLDocument*)ctx;

  if (doc->LoadFile(path.c_str()) != XML_SUCCESS)
  {
    //throw FileNotFoundException(path);
    return false;
  }

  node_level.clear();
  curr_node = doc->RootElement();

  return true;
}

bool XMLContext::Save(const std::string &path)
{
  auto* doc = (tinyxml2::XMLDocument*)ctx;
  if (!doc) return false;
  return doc->SaveFile(path.c_str()) == XML_SUCCESS;
}

bool XMLContext::next()
{
  auto *n = (tinyxml2::XMLElement*)curr_node;
  if (!n) return false;
  curr_node = n->NextSiblingElement();
  return curr_node != 0;
}

bool XMLContext::step_in()
{
  auto *n = (tinyxml2::XMLElement*)curr_node;
  if (!n) return false;
  tinyxml2::XMLElement *child = n->FirstChildElement();
  if (!child) return false;
  node_level.push_back(curr_node);
  curr_node = child;
  return true;
}

bool XMLContext::step_out()
{
  if (node_level.empty()) return false;
  curr_node = node_level.back();
  node_level.pop_back();
  return true;
}

void* XMLContext::get_document()
{
  return ctx;
}

void* XMLContext::get_rootnode()
{
  return ctx ? ((tinyxml2::XMLDocument*)ctx)->RootElement() : nullptr;
}

void* XMLContext::get_node()
{
  return curr_node;
}

MetricGroup XMLContext::GetCurrentMetric() const
{
  MetricGroup g;
  auto *p = (tinyxml2::XMLElement*)curr_node;
  if (p)
  {
    const auto *attr = p->FirstAttribute();
    while (attr)
    {
      g.set(attr->Name(), attr->Value());
      attr = attr->Next();
    }
  }
  return g;
}

// ----------------------------------------------------------------- XMLExecutor

static std::map<std::string, XMLCommandHandler> &getXMLHandler()
{
  static std::map<std::string, XMLCommandHandler> gXMLHandlers;
  return gXMLHandlers;
}

XMLExecutor::XMLExecutor(XMLContext *ctx) : ctx_(ctx) {}

void XMLExecutor::AddHandler(const std::string &cmd, XMLCommandHandler handler)
{
  getXMLHandler()[cmd] = handler;
}

bool XMLExecutor::RunInternal()
{
  while (ctx_->get_node())
  {
    tinyxml2::XMLElement* p = (tinyxml2::XMLElement*)ctx_->get_node();
    auto i = getXMLHandler().find(p->Name());
    if (i != getXMLHandler().end())
      (*i->second)(this, ctx_);
    if (ctx_->step_in())
    {
      parent_.push_back(current_);
      RunInternal();
      parent_.pop_back();
      ctx_->step_out();
    }
    ctx_->next();
  }
  return true;
}

bool XMLExecutor::Run()
{
  bool r = false;
  if (ctx_->step_in())
  {
    parent_.push_back(current_);
    r = RunInternal();
    parent_.pop_back();
    ctx_->step_out();
  }
  return r;
}

void XMLExecutor::SetCurrentObject(void *p)
{
  current_ = p;
}

void* XMLExecutor::GetParent()
{
  return parent_.empty() ? nullptr : parent_.back();
}


// ----------------------------------------------------------------- CSVContext

CSVContext::CSVContext() : sep(','), row_idx(0), col_size(0)
{
  memset(cols, 0, sizeof(cols));
}

CSVContext::~CSVContext()
{
}

void CSVContext::set_separator(char c) { sep = c; }

bool CSVContext::Load(const std::string &path)
{
  std::string r;
  std::ifstream is(path.c_str());
  if (is.fail()) return false;
  rows.clear();
  while (std::getline(is, r))
  {
    rows.push_back(r);
  }
  row_idx = 0;
  next();
  return true;
}

bool CSVContext::next()
{
  unsigned i = 0, ci = 0;
  if (row_idx >= rows.size()) return false;
  auto &r = rows[row_idx];
  memset(cols, 0, sizeof(cols));
  cols[ci++] = r.c_str();
  for (; i < r.size(); ++i)
  {
    if (r[i] == sep)
    {
      r[i] = '\0';
      cols[ci++] = &r[i + 1];
      i++;
    }
  }
  col_size = ci;
  row_idx++;
  return true;
}

const char *CSVContext::get_str(unsigned idx) const
{
  return cols[idx] ? cols[idx] : "";
}

int CSVContext::get_int(unsigned idx) const
{
  const char *c = get_str(idx);
  if (c[0] == '-') return -atoi(c + 1);
  else return atoi(c);
}

float CSVContext::get_float(unsigned idx) const
{
  const char *c = get_str(idx);
  if (c[0] == '-') return -atof(c + 1);
  else return atof(c);
}

unsigned CSVContext::get_col_size() const
{
  return col_size;
}


// -------------------------------------------------------------- LR2CSVContext

LR2CSVContext::LR2CSVContext()
{
}

LR2CSVContext::~LR2CSVContext()
{
}

bool LR2CSVContext::Load(const std::string &path)
{
  ctx.clear();
  return LoadContextStack(path);
}

bool LR2CSVContext::next()
{
  while (!ctx.empty())
  {
    if (!ctx.back().next())
    {
      ctx.pop_back();
      continue;
    }

    auto &c = ctx.back();
    const char *cmd = c.get_str(0);
    R_ASSERT(cmd);

    // preprocess commands
    if (cmd[0] == '#')
    {
      const char *val = c.get_str(1);
      if (stricmp("#IF", cmd) == 0)
      {
        if (*KEYPOOL->GetInt(val)) AddIfStmtStack(false);
        else AddIfStmtStack(true);
        continue; // fetch next line
      }
      else if (stricmp("#ELSEIF", cmd) == 0)
      {
        if (if_stack_.empty())
          continue;
        if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0)
        {
          if_stack_.back().cond_is_true = false;
          continue;
        }

        if (*KEYPOOL->GetInt(val))
        {
          if_stack_.back().cond_is_true = true;
          if_stack_.back().cond_match_count++;
        }
        continue; // fetch next line
      }
      else if (stricmp("#ELSE", cmd) == 0)
      {
        if (if_stack_.empty())
          continue;
        if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0)
        {
          if_stack_.back().cond_is_true = false;
          continue;
        }
        if_stack_.back().cond_is_true = true;
        if_stack_.back().cond_match_count++;
        continue; // fetch next line
      }
      else if (stricmp("#ENDIF", cmd) == 0)
      {
        if (if_stack_.empty())
        {
          Logger::Error("LR2CSVContext: #ENDIF without #IF statement, ignored.");
          break;
        }
        if_stack_.pop_back();
        continue; // fetch next line
      }
    }

    /* if conditional statement failed, cannot pass over it. */
    if (!if_stack_.empty() && !if_stack_.back().cond_is_true)
      continue;

    if (stricmp("#INCLUDE", cmd) == 0)
    {
      LoadContextStack(c.get_str(1));
      continue; // fetch next line
    }

    return true;
  }
  return false;
}

const char *LR2CSVContext::get_str(unsigned idx) const
{
  if (ctx.empty()) return nullptr;
  else return ctx.back().get_str(idx);
}

int LR2CSVContext::get_int(unsigned idx) const
{
  if (ctx.empty()) return 0;
  else return ctx.back().get_int(idx);
}

float LR2CSVContext::get_float(unsigned idx) const
{
  if (ctx.empty()) return .0f;
  else return ctx.back().get_float(idx);
}

bool LR2CSVContext::LoadContextStack(const std::string &path)
{
  ctx.emplace_back(CSVContext());
  if (!ctx.back().Load(SubstitutePath(path)))
  {
    ctx.pop_back();
    return false;
  }
  return true;
}

void LR2CSVContext::AddIfStmtStack(bool cond_is_true)
{
  if_stack_.emplace_back(IfStmt{ cond_is_true ? 1 : 0, cond_is_true });
}

std::string LR2CSVContext::SubstitutePath(const std::string& path)
{
  return Substitute(path, "LR2files/Theme", "themes");
}


// ------------------------------------------------------------- LR2CSVExecutor

static std::map<std::string, LR2CSVCommandHandler> &getLR2CSVHandler()
{
  static std::map<std::string, LR2CSVCommandHandler> gLR2CSVHandlers;
  return gLR2CSVHandlers;
}

LR2CSVExecutor::LR2CSVExecutor(LR2CSVContext *ctx)
  : ctx_(ctx), image_count_(0), font_count_(0), command_index_(0), command_count_(0) {}

LR2CSVExecutor::~LR2CSVExecutor() {}

void LR2CSVExecutor::AddHandler(const std::string &cmd, LR2CSVCommandHandler handler)
{
  getLR2CSVHandler()[cmd] = handler;
}

void LR2CSVExecutor::CallHandler(const std::string &cmd, void *_this,
                                 LR2CSVExecutor *loader, LR2CSVContext *ctx)
{
  auto &i = getLR2CSVHandler().find(cmd);
  if (i != getLR2CSVHandler().end())
    (*i->second)(_this, loader, ctx);
}

void LR2CSVExecutor::Run()
{
  while (ctx_->next())
  {
    const char *cmd = ctx_->get_str(0);
    int index = ctx_->get_int(1);
    auto i = getLR2CSVHandler().find(cmd);
    if (i != getLR2CSVHandler().end())
    {
      if (command_ != cmd || command_index_ != index)
      {
        command_ = cmd;
        command_index_ = index;
        command_count_ = 0;
      }
      (*i->second)(nullptr, this, ctx_);
      command_count_++;
    }
  }
}

unsigned LR2CSVExecutor::get_image_index() { return image_count_++; }

unsigned LR2CSVExecutor::get_font_index() { return font_count_++; }

unsigned LR2CSVExecutor::get_command_index()
{
  return command_count_;
}

void LR2CSVExecutor::reset_command() { command_.clear(); command_count_ = 0; }

void LR2CSVExecutor::set_object(const std::string &name, void *obj)
{
  reset_command();
  objects_[name] = obj;
}

void* LR2CSVExecutor::get_object(const std::string &name)
{
  return objects_[name];
}


namespace Script
{

bool Load(const std::string &path, void *baseobject)
{
  std::string ext = GetExtension(path);
  if (ext == "xml")
  {
    XMLContext ctx;
    if (!ctx.Load(path))
    {
      Logger::Error("XML file load failed: %s", path.c_str());
      return false;
    }
    XMLExecutor loader(&ctx);
    loader.SetCurrentObject(baseobject);
    return loader.Run();
  }
  else if (ext == "lr2skin" || ext == "lr2ss")
  {
    LR2CSVContext ctx;
    if (!ctx.Load(path))
    {
      Logger::Error("LR2CSV file load failed: %s", path.c_str());
      return false;
    }
    LR2CSVExecutor loader(&ctx);
    R_ASSERT(baseobject);
    loader.set_object("scene", baseobject);
    loader.Run();
    return true;
  }
  else
  {
    Logger::Error("Unsupported script: %s", path.c_str());
    return false;
  }
}

}


#if 0
void Script::Execute(const std::string &path)
{
  std::string ext(GetExtension(path));
  if (ext == "lr2skin")
  {
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
#endif

#if 0
constexpr const char* kLR2SubstitutePath = "LR2files/Theme";
constexpr const char* kSubstitutePath = "themes";

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
  NAME("SRC_AUTO_NOTE", "ANote%d", "lr2src", 0), \
  NAME("SRC_LN_END", "NoteLnTail%d", "lr2src", 0), \
  NAME("SRC_AUTO_LN_END", "ANoteLnTail%d", "lr2src", 0), \
  NAME("SRC_LN_START", "NoteLnHead%d", "lr2src", 0), \
  NAME("SRC_AUTO_LN_START", "ANoteLnHead%d", "lr2src", 0), \
  NAME("SRC_LN_BODY", "NoteLnBody%d", "lr2src", 0), \
  NAME("SRC_AUTO_LN_BODY", "ANoteLnBody%d", "lr2src", 0), \
  NAME("SRC_MINE", "NoteMine%d", "lr2src", 0), \
  NAME("DST_NOTE", "NoteField", "Lane%dOnLR", 0), \
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
        object_metrics.push_back(std::tuple<std::string, Metric*, bool>{ name, curr_metrics, true });
      }
    }
    else if (_objtypes[nameidx] == 1)
    {
      curr_metrics = object_processing[name];
    }
    else
    {
      object_metrics.push_back(std::tuple<std::string, Metric*, bool>{ name, new Metric(), false });
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
#endif

}
