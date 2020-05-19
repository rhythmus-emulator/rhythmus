#include "Error.h"
#include "Logger.h"       /* For logging when error occurs */
#include "common.h"

namespace rhythmus
{

RuntimeException::RuntimeException(const std::string& msg)
  : msg_(msg), is_ignorable_(false) {}

const char* RuntimeException::exception_name() const noexcept
{
  return "RuntimeException";
}

const char* RuntimeException::what() const noexcept
{
  return msg_.c_str();
}

void RuntimeException::set_ignorable(bool ignorable)
{
  is_ignorable_ = ignorable;
}
bool RuntimeException::is_ignorable() const
{
  return is_ignorable_;
}

FatalException::FatalException(const std::string &msg) : RuntimeException(msg) {}

const char* FatalException::exception_name() const noexcept
{
  return "FatalException";
}

UnimplementedException::UnimplementedException(const std::string& msg)
  : RuntimeException(msg) {}

const char* UnimplementedException::exception_name() const noexcept
{
  return "UnimplementedException";
}

RetryException::RetryException(const std::string& msg)
  : RuntimeException(msg) {}

const char* RetryException::exception_name() const noexcept
{
  return "RetryException";
}

FileNotFoundException::FileNotFoundException(const std::string& filename)
  : RuntimeException("File not found: " + filename) {}

const char* FileNotFoundException::exception_name() const noexcept
{
  return "FileNotFoundException";
}

void R_THROW(bool v)
{
  static const std::string& msg("exception thrown");
  R_THROW(v, msg);
}

void R_THROW(bool v, const char* msg)
{
  if (!v) throw RuntimeException(msg ? msg : "");
}

void R_THROW(bool v, const std::string& msg)
{
  if (!v) throw RuntimeException(msg);
}

void R_THROW_FATAL(bool v, const char* msg)
{
  if (!v) throw FatalException(msg ? msg : "");
}

void R_THROW_FATAL(bool v, const std::string& msg)
{
  if (!v) throw FatalException(msg);
}

}
