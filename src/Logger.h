#pragma once

#include <string>
#include <stdarg.h>
#include <list>
#include <sstream>

namespace rhythmus
{

class Logger
{
public:
  static void Initialize();

  void HookStdOut();
  void UnHookStdOut();
  void Flush();
  void StartLogging();
  void FinishLogging();
  void Log(int level, const char* msg, ...);
  std::string GetRecentLog();

  static Logger& getInstance();
  static void Info(const char* msg, ...);
  static void Warn(const char* msg, ...);
  static void Error(const char* msg, ...);

private:
  void Log_Internal(int level, const char* msg, va_list l);
  void AddToRecentLog(const std::string& l);

  std::string log_path_;
  bool append_log_;
  FILE *f_;
  std::list<std::string> recent_logs_;
  std::streambuf *org_cerr_buf_, *org_cout_buf_;
  std::stringstream cerr_ss_, cout_ss_;
  Logger();
  ~Logger();
};

}