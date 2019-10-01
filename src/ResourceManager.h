#pragma once

#include "Image.h"
#include "Font.h"
#include <string>
#include <vector>
#include <map>
#include <list>

namespace rhythmus
{

/**
 * @brief
 * Contains cached resource(image, sound, etc.) objects
 */
class ResourceManager
{
public:
  static ImageAuto LoadImage(const std::string& path);
  static FontAuto LoadFont(const std::string& path, FontAttributes& attrs);
  static FontAuto LoadLR2Font(const std::string& path);
  static FontAuto GetSystemFont();
  static void ReleaseImage(ImageAuto img);
  static void ReleaseFont(const FontAuto& font);

  static ResourceManager& getInstance();

  static void CacheSystemDirectory();

  /**
   * @brief
   * Cache a folder and it's subdirectory path.
   * (considering as it's game system directory)
   * Cached paths are used for masked-path searching.
   */
  void CacheDirectory(const std::string& dir);

  /**
   * @brief
   * Set higher priority for specific cached file path (if exists).
   * Used when specific file should be matched first than others.
   */
  void MakePathHigherPriority(const std::string& path);

  /**
   * @brief
   * Get cached file path masked path.
   * Result is returned as input is if it's not masked path or not found.
   */
  std::string GetPath(const std::string& masked_path, bool &is_found) const;
  std::string GetPath(const std::string& masked_path) const;

  /* @brief get all available paths from cached path */
  void GetAllPaths(const std::string& masked_path, std::vector<std::string> &out) const;

  /* @brief add path replacement for some special use */
  void AddPathReplacement(const std::string& path_from, const std::string& path_to);

private:
  ResourceManager();
  ~ResourceManager();

  // cached font
  std::list<FontAuto> fonts_;

  // cached paths
  std::vector<std::string> path_cached_;

  // path replacement
  std::map<std::string, std::string> path_replacement_;
};

}