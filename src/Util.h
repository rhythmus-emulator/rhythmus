#pragma once

#include <string>
#include <vector>

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

/* @brief string format util */
template <typename ... Args>
std::string format_string(const std::string& format, Args ... args)
{
  size_t size = snprintf(nullptr, 0, format.c_str(), args ...);
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args ...);
  return std::string(buf.get(), buf.get() + size - 1);
}

std::string GetExtension(const std::string& path);

std::string Substitute(const std::string& org, const std::string& startswith, const std::string& relplacewith);

std::string Replace(const std::string& org, const std::string& target, const std::string& replaceto);

void Split(const std::string& str, char sep, std::vector<std::string>& vsOut);

RHYTHMUS_NAMESPACE_END