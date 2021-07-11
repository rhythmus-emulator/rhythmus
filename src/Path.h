#pragma once

#include "common.h"
#include "Util.h"

namespace rhythmus
{
  
/* @brief Cache directory/files managed by program. */
class PathCache
{
public:
  PathCache();

  static void Initialize();

  /* Add virtual symbolic link, specially for LR2 compatibility. */
  static void AddLR2SymLink();

  /**
   * Get cached file path masked path.
   * Result is returned as input is if it's not masked path or not found.
   */
  std::string GetPath(const std::string& masked_path, bool &is_found) const;
  std::string GetPath(const std::string& masked_path) const;

  /* get all available paths from cache */
  void GetAllPaths(const std::string& masked_path, std::vector<std::string> &out) const;

  /* get all available directories from cache*/
  void GetDirectories(const std::string& masked_path, std::vector<std::string>& out) const;
  void GetDescendantDirectories(const std::string& dirpath, std::vector<std::string>& out) const;

  /* add path replacement for some special use */
  void SetSymbolLink(const std::string& path_from, const std::string& path_to);
  void RemoveSymbolLink(const std::string& path_src);

  /**
   * Cache a folder recursively.
   * (considering as it's game system directory)
   * Cached paths are used for masked-path searching.
   */
  void CacheDirectory(const std::string& dir);

  void CacheSystemDirectory();

  /**
   * @brief
   * Set higher priority for specific cached file path (if exists).
   * Used when specific file should be matched first than others.
   */
  void MakePathHigherPriority(const std::string& path);

private:
  // cached paths
  std::vector<DirItem> path_cached_;

  // path replacement (virtual symbol link)
  std::map<std::string, std::string> path_replacement_;

  // resolve masked path to cached one.
  // if not found, empty string returned.
  std::string ResolveMaskedPath(const std::string &path) const;
  
  // process symbol link and path separator.
  std::string PreprocessPath(const std::string &path) const;
};


class FilePath {
public:
  FilePath(const std::string& path);
  bool valid() const;
  const std::string& get() const;
  const char* get_cstr() const;
private:
  std::string path_;
  bool is_valid_;
};

class DirPath {
public:
  DirPath(const std::string& path);
  bool valid() const;
  const std::string& get() const;
  const char* get_cstr() const;
private:
  std::string path_;
  bool is_valid_;
};

extern PathCache *PATH;

}