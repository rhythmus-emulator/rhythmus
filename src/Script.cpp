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

// Whether to execute script only for metadata.
static bool MODE_PRELOAD = false;

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

XMLExecutor::XMLExecutor(XMLContext *ctx) : ctx_(ctx), current_(nullptr) {}

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
  path_ = path;
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

    if (stricmp("#INCLUDE", cmd) == 0 && MODE_PRELOAD == false)
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

const std::string& LR2CSVContext::get_path()
{
  return path_;
}


// ------------------------------------------------------------- LR2CSVExecutor

struct LR2CSVHandler
{
  LR2CSVHandlerFunc func;
  std::vector<std::string> trigger;
  std::vector<std::string> trigger_after;
};

static std::map<std::string, LR2CSVHandler> &getLR2CSVHandler()
{
  static std::map<std::string, LR2CSVHandler> gLR2CSVHandlers;
  return gLR2CSVHandlers;
}

LR2CSVExecutor::LR2CSVExecutor(LR2CSVContext *ctx)
  : ctx_(ctx), image_count_(0), font_count_(0), command_index_(0), command_count_(0) {}

LR2CSVExecutor::~LR2CSVExecutor() {}

void LR2CSVExecutor::AddHandler(const std::string &cmd, LR2CSVHandlerFunc handler)
{
  getLR2CSVHandler()[cmd].func = handler;
}

void LR2CSVExecutor::AddTrigger(const std::string& cmd, const std::string& callback)
{
  getLR2CSVHandler()[cmd].trigger.push_back(callback);
}

void LR2CSVExecutor::AddTriggerAfter(const std::string& cmd, const std::string& callback)
{
  getLR2CSVHandler()[cmd].trigger_after.push_back(callback);
}

static bool _ExecuteHandler(LR2CSVHandler& handler, void *&o, LR2CSVExecutor* e, LR2CSVContext* ctx)
{
  // Check trigger before execution
  for (const auto& cmd : handler.trigger) {
    auto& i = getLR2CSVHandler().find(cmd);
    if (i != getLR2CSVHandler().end())
      if (!_ExecuteHandler(i->second, o, e, ctx))
        return false;
  }

  // Run main function
  if (handler.func)
    if (!(handler.func)(o, e, ctx))
      return false;

  // Check trigger after execution
  for (const auto& cmd : handler.trigger_after) {
    auto& i = getLR2CSVHandler().find(cmd);
    if (i != getLR2CSVHandler().end())
      if (!_ExecuteHandler(i->second, o, e, ctx))
        return false;
  }

  return true;
}

void LR2CSVExecutor::CallHandler(const std::string &cmd, void *o, LR2CSVExecutor *loader, LR2CSVContext *ctx)
{
  auto &i = getLR2CSVHandler().find(cmd);
  if (i != getLR2CSVHandler().end())
    _ExecuteHandler(i->second, o, loader, ctx);
}

void LR2CSVExecutor::Run()
{
  // reset environment
  current_obj_ = nullptr;

  while (ctx_->next()) {
    const char *cmd = ctx_->get_str(0);
    int index = ctx_->get_int(1);
    if (!*cmd)
      continue;
    // Don't run SRC/DST command if running in preload mode
    if (MODE_PRELOAD && (strncmp(cmd, "#SRC", 4) == 0 || strncmp(cmd, "#DST", 4) == 0))
      continue;
    auto i = getLR2CSVHandler().find(cmd);
    if (i != getLR2CSVHandler().end()) {
      // Check is command is changed
      if (command_ != cmd || command_index_ != index) {
        command_ = cmd;
        command_index_ = index;
        command_count_ = 0;
      }

      _ExecuteHandler(i->second, current_obj_, this, ctx_);

      // Add function execution count (continually)
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

void LR2CSVExecutor::set_current_obj(void* obj) { current_obj_ = obj; }

void* LR2CSVExecutor::get_current_obj() { return current_obj_; }

void LR2CSVExecutor::set_object(const std::string &name, void *obj)
{
  // XXX: Unused?
  reset_command();
  objects_[name] = obj;
}

void* LR2CSVExecutor::get_object(const std::string &name)
{
  // XXX: Unused?
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
    if (!ctx.Load(path)) {
      Logger::Error("LR2CSV file load failed: %s", path.c_str());
      return false;
    }
    LR2CSVExecutor loader(&ctx);
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

void SetPreloadMode(bool is_preload)
{
  MODE_PRELOAD = is_preload;
}

} /* namespace Script */

} /* namespace rhythmus */