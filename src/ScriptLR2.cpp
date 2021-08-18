#include "ScriptLR2.h"
#include "Logger.h"
#include "KeyPool.h"

// objects
#include "BaseObject.h"
#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Button.h"
#include "object/Slider.h"
#include "object/Bargraph.h"
#include "object/OnMouse.h"

#include "scene/SelectScene.h"
#include "scene/PlayScene.h"
#include "object/MusicWheel.h"

namespace rhythmus
{

// ------------------------------------------------------------------ LR2FnArgs

static const char* __dummyargs[MAX_CSV_COL];

LR2FnArgs::LR2FnArgs() : cols_(__dummyargs), size_(0) {}

LR2FnArgs::LR2FnArgs(const char* const* cols, unsigned col_size)
  : cols_(cols), size_(col_size) {}

const char* LR2FnArgs::get_str(unsigned idx) const
{
  if (idx >= size_) return nullptr;
  return cols_[idx];
}

int LR2FnArgs::get_int(unsigned idx) const
{
  if (idx >= size_) return 0;
  const char* c = cols_[idx];
  if (c[0] == '-') return -atoi(c + 1);
  else return atoi(c);
}

float LR2FnArgs::get_float(unsigned idx) const
{
  if (idx >= size_) return .0f;
  const char* c = cols_[idx];
  if (c[0] == '-') return -atof(c + 1);
  else return atof(c);
}

unsigned LR2FnArgs::size() const { return size_; }


// -------------------------------------------------------------- LR2CSVContext

LR2CSVContext::LR2CSVContext() : enable_include_(true)
{
}

LR2CSVContext::~LR2CSVContext()
{
}

bool LR2CSVContext::Load(const std::string &path)
{
  return Load(FilePath(path));
}

bool LR2CSVContext::Load(const FilePath& path)
{
  ctx.clear();
  path_ = path.get();
  folder_ = GetFolderPath(path_);
  return LoadContextStack(path_);
}

bool LR2CSVContext::next()
{
  while (!ctx.empty()) {
    if (!ctx.back().next()) {
      ctx.pop_back();
      continue;
    }

    auto &c = ctx.back();
    const char *cmd = c.get_str(0);
    R_ASSERT(cmd != nullptr);

    // preprocess commands
    if (cmd[0] == '#') {
      const char *val = c.get_str(1);
      if (stricmp("#IF", cmd) == 0) {
        if (*KEYPOOL->GetInt(val)) AddIfStmtStack(false);
        else AddIfStmtStack(true);
        continue; // fetch next line
      }
      else if (stricmp("#ELSEIF", cmd) == 0) {
        if (if_stack_.empty())
          continue;
        if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0) {
          if_stack_.back().cond_is_true = false;
          continue;
        }
        if (*KEYPOOL->GetInt(val)) {
          if_stack_.back().cond_is_true = true;
          if_stack_.back().cond_match_count++;
        }
        continue; // fetch next line
      }
      else if (stricmp("#ELSE", cmd) == 0) {
        if (if_stack_.empty())
          continue;
        if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0) {
          if_stack_.back().cond_is_true = false;
          continue;
        }
        if_stack_.back().cond_is_true = true;
        if_stack_.back().cond_match_count++;
        continue; // fetch next line
      }
      else if (stricmp("#ENDIF", cmd) == 0) {
        if (if_stack_.empty()) {
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

    if (stricmp("#INCLUDE", cmd) == 0 && enable_include_) {
      LoadContextStack(FilePath(c.get_str(1)));
      continue; // fetch next line
    }

    break;
  }

  if (ctx.empty()) {
    args_ = LR2FnArgs();
    return false;
  }
  else {
    args_ = LR2FnArgs(ctx.back().get_row(), ctx.back().get_col_size());
    return true;
  }
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

bool LR2CSVContext::LoadContextStack(const FilePath& path)
{
  ctx.emplace_back(CSVContext());
  if (!ctx.back().Load(path)) {
    ctx.pop_back();
    return false;
  }
  return true;
}

void LR2CSVContext::AddIfStmtStack(bool cond_is_true)
{
  if_stack_.emplace_back(IfStmt{ cond_is_true ? 1 : 0, cond_is_true });
}

const std::string& LR2CSVContext::get_path()
{
  return path_;
}

const LR2FnArgs& LR2CSVContext::args() const { return args_; }

void LR2CSVContext::SetEnableInclude(bool v) { enable_include_ = v; }


// ------------------------------------------------------------- LR2CSVExecutor

static std::map<std::string, LR2FnClass*> &getLR2FnClassMap()
{
  static std::map<std::string, LR2FnClass*> gLR2FnClassMap;
  return gLR2FnClassMap;
}

LR2CSVExecutor::LR2CSVExecutor(LR2CSVContext *ctx) : ctx_(ctx) {}

LR2CSVExecutor::~LR2CSVExecutor() {}

void LR2CSVExecutor::AddClass(LR2FnClass* fnclass)
{
  getLR2FnClassMap()[fnclass->GetClassName()] = fnclass;
}

LR2FnClass* LR2CSVExecutor::GetClass(const std::string& type)
{
  auto i = getLR2FnClassMap().find(type);
  if (i != getLR2FnClassMap().end()) return i->second;
  else return nullptr;
}

bool LR2CSVExecutor::RunMethod(const std::string& type, const std::string& method, void *o, const LR2FnArgs& args)
{
  std::string currtype = type;
  while (!currtype.empty()) {
    auto* fnClass = GetClass(currtype);
    if (fnClass == nullptr) break;
    auto fn = fnClass->GetMethod(method);
    if (fn) {
      fn(o, args);
      return true;
    }
    else currtype = fnClass->GetBaseClassName();
  }
  return false;
}

/*
void LR2CSVExecutor::Cleanup()
{
  // static class; DO NOT DELETE
  for (auto i : getLR2FnClassMap()) {
    delete i.second;
  }
  getLR2FnClassMap().clear();
}
*/

void LR2CSVExecutor::Run(void* obj, const std::string& type)
{
  while (ctx_->next()) {
    const char *cmd = ctx_->get_str(0);
    int index = ctx_->get_int(1);
    if (!*cmd) continue;
    LR2CSVExecutor::RunMethod(type, ctx_->get_str(0), obj, ctx_->args());
  }
}

// ----------------------------------------------- LR2FnClass

LR2FnClass::LR2FnClass(const std::string& classname) : name_(classname)
{
  LR2CSVExecutor::AddClass(this);
}

LR2FnClass::LR2FnClass(const std::string& classname, const std::string& baseclassname)
  : name_(classname), basename_(baseclassname)
{
  LR2CSVExecutor::AddClass(this);
}

const std::string& LR2FnClass::GetClassName() const { return name_; }

const std::string& LR2FnClass::GetBaseClassName() const { return basename_; }

void LR2FnClass::AddMethod(const std::string& cmdname, LR2Fn fn) {
  fnmap_[cmdname] = fn;
}

LR2Fn LR2FnClass::GetMethod(const std::string& cmdname) {
  auto i = fnmap_.find(cmdname);
  if (i == fnmap_.end()) return nullptr;
  return i->second;
}

}