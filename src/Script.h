#pragma once

#include "Setting.h"  // MetricGroup
#include <string>
#include <map>
#include <vector>

namespace rhythmus
{
  
/* @brief An context used for load/save XML file */
class XMLContext
{
public:
  XMLContext();
  XMLContext(const std::string &rootnodename);
  virtual ~XMLContext();
  bool Load(const std::string &path);
  bool Save(const std::string &path);
  bool next();
  bool step_in();
  bool step_out();
  void* get_document();
  void* get_rootnode();
  void* get_node();
  MetricGroup GetCurrentMetric() const;

private:
  void *ctx;
  void *curr_node;
  std::vector<void*> node_level;
};

class XMLExecutor;

/* @brief base handler of XML */
typedef void (*XMLCommandHandler)(XMLExecutor *e, XMLContext *ctx);

class XMLExecutor
{
public:
  XMLExecutor(XMLContext *ctx);
  static void AddHandler(const std::string &cmd, XMLCommandHandler *handler);
  bool Run();
  void SetCurrentObject(void *p);
  void* GetParent();
private:
  XMLContext *ctx_;
  void *current_;
  std::vector<void*> parent_;
};

/* @brief An context used for load csv file */
class CSVContext
{
public:
  CSVContext();
  virtual ~CSVContext();
  void set_separator(char c);
  bool Load(const std::string &path);
  bool next();
  const char *get_str(unsigned idx) const;
  int get_int(unsigned idx) const;
  float get_float(unsigned idx) const;
  unsigned get_col_size() const;

private:
  std::vector<std::string> rows;
  char sep;
  unsigned row_idx;
  unsigned col_size;
  const char *cols[256];
};

/* @brief An context used for load lr2csv file (with command support) */
class LR2CSVContext
{
public:
  LR2CSVContext();
  virtual ~LR2CSVContext();
  bool Load(const std::string &path);
  bool next();
  const char *get_str(unsigned idx) const;
  int get_int(unsigned idx) const;
  float get_float(unsigned idx) const;

private:
  std::vector<CSVContext> ctx;

  struct IfStmt
  {
    int cond_match_count;
    bool cond_is_true;
  };

  std::vector<IfStmt> if_stack_;

  bool LoadContextStack(const std::string &path);
  void AddIfStmtStack(bool cond_is_true);
};

class LR2CSVExecutor;

/* @brief base handler of LR2CSV */
typedef void (*LR2CSVCommandHandler)(void *_this, LR2CSVExecutor *loader, LR2CSVContext *ctx);

/* @brief executes LR2CSV with registered handler. */
class LR2CSVExecutor
{
public:
  LR2CSVExecutor(LR2CSVContext *ctx);
  ~LR2CSVExecutor();
  static void AddHandler(const std::string &cmd, LR2CSVCommandHandler *handler);
  static void CallHandler(const std::string &cmd, void *_this,
                          LR2CSVExecutor *loader, LR2CSVContext *ctx);
  virtual void Run();

  unsigned get_image_index();
  unsigned get_font_index();
  unsigned get_command_index();
  void reset_command();
  void set_object(const std::string &name, void *obj);
  void* get_object(const std::string &name);
private:
  LR2CSVContext *ctx_;
  unsigned image_count_;
  unsigned font_count_;
  std::string command_;
  unsigned command_count_;
  std::map<std::string, void*> objects_;
};

/* @brief executes Lua script. script context is remained while program is run. */
class LuaExecutor
{
};

namespace Script
{
  bool Load(const std::string &path, void *baseobject);
}

}