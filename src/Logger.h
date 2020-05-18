#pragma once

#include <string>
#include <stdarg.h>
#include <list>
#include <sstream>

namespace rhythmus
{

enum LogMode
{
  kLogInfo,
  kLogWarn,
  kLogError
};

class Logger
{
public:
  void Initialize();

  void HookStdOut();
  void UnHookStdOut();
  void Flush();
  void StartLogging();
  void FinishLogging();
  void Log(int level, const char* msg, ...);
  std::string GetRecentLog();

  static void Info(const char* msg, ...);
  static void Warn(const char* msg, ...);
  static void Error(const char* msg, ...);
  static Logger& getInstance();
  static Logger& getInstance(int default_log_mode);
  typedef Logger& (*LoggerManipulator)(Logger&);
  Logger& operator<<(LoggerManipulator manip)
  {
    return manip(*this);
  }
  Logger& operator<<(const char* v);
  Logger& operator<<(const std::string &v);
  Logger& operator<<(int v);
  Logger& operator<<(double v);
  Logger& operator<<(float v);
  static Logger& endl(Logger &logger);

private:
  void Log_Internal(int level, const char* msg, va_list l);
  void AddToRecentLog(const std::string& l);

  bool enable_logging_;
  std::string log_path_;
  bool append_log_;
  int log_mode_;
  FILE *f_;
  std::list<std::string> recent_logs_;
  std::streambuf *org_cerr_buf_, *org_cout_buf_;
  std::stringstream cerr_ss_, cout_ss_;
  std::stringstream logger_ss_;
  Logger();
  ~Logger();
};

}