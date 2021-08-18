#pragma once

#include "Setting.h"  // MetricGroup
#include "Path.h"     // FilePath
#include <string>
#include <map>
#include <vector>

#define MAX_CSV_COL 256

namespace rhythmus
{
  
/* @brief An context used for load/save XML file */
class XMLContext
{
public:
  XMLContext();
  XMLContext(const std::string &rootnodename);
  virtual ~XMLContext();
  bool Load(const FilePath& path);
  bool Load(const std::string &path);
  bool Save(const FilePath& path);
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
  static void AddHandler(const std::string &cmd, XMLCommandHandler handler);
  bool Run();
  void SetCurrentObject(void *p);
  void* GetParent();
private:
  bool RunInternal();
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
  bool Load(const FilePath& path);
  bool Load(const std::string &path);
  bool next();
  const char* get_str(unsigned idx) const;
  const char* const* get_row() const;
  int get_int(unsigned idx) const;
  float get_float(unsigned idx) const;
  unsigned get_col_size() const;

private:
  std::vector<std::string> rows;
  char sep;
  unsigned row_idx;
  unsigned col_size;
  const char *cols[MAX_CSV_COL];
};

/* @brief executes Lua script. script context is remained while program is run. */
class LuaExecutor
{
};

namespace Script
{
  bool Load(const FilePath& path, void* baseobject, const std::string& type);
  bool Load(const std::string &path, void *baseobject, const std::string& type);
}

}