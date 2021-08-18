#pragma once
#include "Script.h"

namespace rhythmus
{

class LR2CSVExecutor;
class LR2CSVContext;
class LR2FnArgs;
class LR2FnClass;

typedef void (*LR2Fn)(void*, const LR2FnArgs&);
using LR2FnMap = std::map<std::string, LR2Fn>;

class LR2FnArgs {
public:
  LR2FnArgs();
  LR2FnArgs(const char* const* cols, unsigned col_size);
  const char* get_str(unsigned idx) const;
  int get_int(unsigned idx) const;
  float get_float(unsigned idx) const;
  unsigned size() const;
private:
  const char* const* cols_;
  unsigned size_;
};

/* @brief An context used for load lr2csv file (with command support) */
class LR2CSVContext
{
public:
  LR2CSVContext();
  virtual ~LR2CSVContext();
  bool Load(const std::string& path);
  bool Load(const FilePath& path);
  bool next();
  const char* get_str(unsigned idx) const;
  int get_int(unsigned idx) const;
  float get_float(unsigned idx) const;
  const std::string& get_path();
  const LR2FnArgs& args() const;
  void SetEnableInclude(bool v);

private:
  std::vector<CSVContext> ctx;
  std::string path_;
  std::string folder_;
  LR2FnArgs args_;

  struct IfStmt
  {
    int cond_match_count;
    bool cond_is_true;
  };

  bool enable_include_;
  std::vector<IfStmt> if_stack_;

  bool LoadContextStack(const FilePath& path);
  void AddIfStmtStack(bool cond_is_true);
};

class LR2CSVExecutor;


/* @brief base handler of LR2CSV */
typedef void (*LR2CSVHandlerFunc)(void*, LR2CSVExecutor* loader, LR2CSVContext* ctx);

/* @brief executes LR2CSV with registered handler. */
class LR2CSVExecutor
{
public:
  LR2CSVExecutor(LR2CSVContext* ctx);
  ~LR2CSVExecutor();
  static void AddClass(LR2FnClass* fnclass);
  static LR2FnClass* GetClass(const std::string& type);
  static bool RunMethod(const std::string& type, const std::string& method, void *o, const LR2FnArgs& args);
  void Run(void *obj, const std::string& type);
private:
  LR2CSVContext* ctx_;
  std::string command_;
};

class LR2FnClass {
public:
  LR2FnClass(const std::string& classname);
  LR2FnClass(const std::string& classname, const std::string& baseclassname);
  const std::string& GetClassName() const;
  const std::string& GetBaseClassName() const;
  void AddMethod(const std::string& cmdname, LR2Fn fn);
  LR2Fn GetMethod(const std::string& cmdname);
private:
  std::string name_;
  std::string basename_;
  LR2FnMap fnmap_;
};

template <class T>
const char* GetTypename();

#define REGISTER_LR2OBJECT(classname) \
  template <> const char* GetTypename<classname>() { return #classname; } \
  void classname::RunLR2Command(const std::string& command, const LR2FnArgs& args) { \
    LR2CSVExecutor::RunMethod(#classname, command, this, args); \
  }

#define HANDLERLR2(_name_) \
  static void _name_ (HANDLERLR2_OBJNAME *o, const LR2FnArgs& args)

#define ADDHANDLERLR2(_name_) \
  LR2FnClass::AddMethod(#_name_, (LR2Fn)_name_);

#define ADDSHANDLERLR2(_name_) \
  LR2FnClass::AddMethod("#" #_name_, (LR2Fn)_name_);

}