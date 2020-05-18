#ifdef _MSC_VER
# define _CRT_NO_VA_START_VALIDATION
#endif

#include "Logger.h"
#include "Setting.h"
#include "Game.h"
#include "Timer.h"
#include "common.h"

namespace rhythmus
{

struct cout_redirect {
  cout_redirect(std::streambuf * new_buffer)
    : old(std::cout.rdbuf(new_buffer))
  { }

  ~cout_redirect() {
    std::cout.rdbuf(old);
  }

private:
  std::streambuf * old;
};

Logger::Logger()
  : enable_logging_(true), log_path_("log/log.txt"), append_log_(false), log_mode_(LogMode::kLogError), f_(0)
{
}

Logger::~Logger()
{
  UnHookStdOut();
  FinishLogging();
}

void Logger::Initialize()
{
  // fetch preference
  enable_logging_ = *PREFERENCE->logging;

  // start logging
  if (enable_logging_)
  {
    StartLogging();
    HookStdOut();
  }
}

void Logger::HookStdOut()
{
  org_cout_buf_ = std::cout.rdbuf(cout_ss_.rdbuf());
  org_cerr_buf_ = std::cerr.rdbuf(cerr_ss_.rdbuf());
}

void Logger::UnHookStdOut()
{
  std::cout.rdbuf(org_cout_buf_);
  std::cout.rdbuf(org_cerr_buf_);
}

void Logger::Flush()
{
  // Flush stdout hooked message to logger and clear buffer
  std::string so, se;
  so = cout_ss_.str();
  se = cerr_ss_.str();
  if (so.size())
  {
    Info(so.c_str());
  }
  if (se.size())
  {
    Error(se.c_str());
  }

  // clear buffer
  cout_ss_.str("");
  cerr_ss_.str("");

  // flush file
  if (f_) fflush(f_);
}

void Logger::StartLogging()
{
  FinishLogging();
  const char *mode = "w";
  f_ = fopen(log_path_.c_str(), mode);
  if (!f_) return;  // log starting failed
  Info("Logging started.");
}

void Logger::FinishLogging()
{
  if (f_)
  {
    Flush();
    fclose(f_);
    f_ = 0;
  }
}

std::string Logger::GetRecentLog()
{
  std::string r;
  for (const auto& s : recent_logs_)
  {
    r += s + '\n';
  }
  return r;
}

void Logger::Log(int level, const char* msg, ...)
{
  va_list args;
  va_start(args, msg);
  getInstance().Log_Internal(level, msg, args);
  va_end(args);
}

void Logger::Info(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);
  getInstance().Log_Internal(LogMode::kLogInfo, msg, args);
  va_end(args);
}

void Logger::Warn(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);
  getInstance().Log_Internal(LogMode::kLogWarn, msg, args);
  va_end(args);
}

void Logger::Error(const char* msg, ...)
{
  va_list args;
  va_start(args, msg);
  getInstance().Log_Internal(LogMode::kLogError, msg, args);
  va_end(args);
}

void Logger::Log_Internal(int level, const char* msg, va_list l)
{
  if (msg[0] == 0) return;

  char time_str[64];
  uint32_t cur_time = Timer::SystemTimer().GetTimeInMillisecond();
  uint32_t
    th = cur_time / 3600 / 1000 % 100,
    tm = cur_time / 60 / 1000 % 60,
    ts = cur_time / 1000 % 60;
  sprintf(time_str, "%02u:%02u:%02u ", th, tm, ts);

  char buf[1024];
  switch (level)
  {
  case LogMode::kLogInfo:
    strcpy(buf, "[INFO] ");
    break;
  case LogMode::kLogWarn:
    strcpy(buf, "[WARN] ");
    break;
  case LogMode::kLogError:
    strcpy(buf, "[ERROR] ");
    break;
  }
  strcpy(buf + strlen(buf), time_str);
  vsnprintf(buf + strlen(buf), 968, msg, l);

  if (f_)
  {
    fwrite(buf, 1, strlen(buf), f_);
    fwrite("\n", 1, 1, f_);
  }
  AddToRecentLog(buf);
}

void Logger::AddToRecentLog(const std::string& l)
{
  recent_logs_.push_back(l);
  while (recent_logs_.size() > 10)
    recent_logs_.pop_front();
}

Logger& Logger::getInstance()
{
  static Logger logger_;
  return logger_;
}

Logger& Logger::getInstance(int default_log_mode)
{
  auto &logger = getInstance();
  logger.log_mode_ = default_log_mode;
  logger.logger_ss_.clear();
  return logger;
}

Logger& Logger::operator<<(const char* v)
{
  logger_ss_ << v;
  return *this;
}

Logger& Logger::operator<<(const std::string &v)
{
  logger_ss_ << v;
  return *this;
}

Logger& Logger::operator<<(int v)
{
  logger_ss_ << v;
  return *this;
}

Logger& Logger::operator<<(double v)
{
  logger_ss_ << v;
  return *this;
}

Logger& Logger::operator<<(float v)
{
  logger_ss_ << v;
  return *this;
}

Logger& Logger::endl(Logger &logger)
{
  va_list args; // dummy
  logger.Log_Internal(logger.log_mode_, logger.logger_ss_.str().c_str(), args);
  return logger;
}

}
