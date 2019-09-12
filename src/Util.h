#pragma once

#include <string>
#include <vector>

namespace rhythmus
{

/* @brief Directory item info */
struct DirItem
{
  std::string filename;
  int is_file;
  int64_t timestamp_modified;
};

/* @brief Get all items in a directory (without recursive) */
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out);

}