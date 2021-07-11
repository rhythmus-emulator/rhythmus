#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

/* To prevent indentation */
#define RHYTHMUS_NAMESPACE_BEGIN namespace rhythmus {
#define RHYTHMUS_NAMESPACE_END   }

RHYTHMUS_NAMESPACE_BEGIN

/* @brief Directory item info */
struct DirItem
{
  std::string filename;
  int is_file;
  int64_t timestamp_modified;
};

/* @brief Get all items in a directory (without recursive) */
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out);
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out, bool recursive);

/* @brief string format util */
template <typename ... Args>
std::string format_string(const std::string& format, Args ... args)
{
  size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

std::string GetExtension(const std::string& path);

std::string GetFolderPath(const std::string& path);

std::string Lower(const std::string &s);

std::string Upper(const std::string &s);

std::string Substitute(const std::string& org, const std::string& startswith, const std::string& relplacewith);

std::string Replace(const std::string& org, const std::string& target, const std::string& replaceto);

bool startsWith(const std::string &s, const std::string &pattern);

bool endsWith(const std::string &s, const std::string &pattern);

void Split(const std::string& str, char sep, std::vector<std::string>& vsOut);

void Split(const std::string& str, char sep, std::string &s1, std::string &s2);

bool IsFile(const std::string& path);

bool IsDirectory(const std::string& path);

void GetDirectoriesFromPath(const std::string& dir_path, std::vector<std::string> &out);

void GetFilesFromPath(const std::string& file_or_filter_path, std::vector<std::string> &out);

void GetFilesFromDirectory(const std::string& dir, std::vector<std::string> &out, int depth);

/**
 * @brief Get random file (or specified index) from filtered path.
 *        Return false if any suitable file is found.
 *        If existing file path, then it will always return true.
 */
bool GetFilepathSmart(const std::string& file_or_filter_path, std::string& out, int index = -1);

/**
 * @brief Extension of GetFilepathSmart() function with fallback path.
 *        If file fails, then search for fallback path (filtering support).
 */
bool GetFilepathSmartFallback(const std::string& file_path, const std::string& fallback, std::string& out, int index = -1);

void MakeParamCountSafe(std::vector<std::string> &v, size_t expected_count);

void MakeParamCountSafe(const std::string &in, std::vector<std::string> &v, size_t expected_count);

std::string GetFirstParam(const std::string& in, char sep = ',');

bool CheckMasking(const std::string& path, const std::string& mask);

bool CompareFilename(const std::string &path1, const std::string &path2);

size_t FindUtf8FirstByteReversed(const std::string &s);

void ConvertUTF32ToUTF8(uint32_t u32, char *out, unsigned *len);

#if WIN32
std::string GetUtf8FromWString(const std::wstring& wstring);
#endif

constexpr size_t kMaxCommandArgs = 128;

/** @brief Argument for object command. */
class CommandArgs
{
public:
  CommandArgs() = default;
  CommandArgs(const std::string &argv);
  CommandArgs(const std::string &argv, size_t arg_count, bool fit_size);

  void Parse(const std::string &argv);
  void Parse(const std::string &argv, size_t arg_count, bool fit_size);
  void set_separator(char sep);
  void set_trim(bool enable);

  template <typename T>
  T Get(size_t arg_index) const;

  const char* Get_str(size_t arg_index) const;

  size_t size() const;

private:
  std::string s_;
  char sep_;
  const char *args_[kMaxCommandArgs];
  size_t len_;
  bool trim_;
};

void Sleep(size_t milisecond);

/* we need these functions as it's not C99 standard */
#ifndef _MSC_VER
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, size_t len);
#endif

RHYTHMUS_NAMESPACE_END
