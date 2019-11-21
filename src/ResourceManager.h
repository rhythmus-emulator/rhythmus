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
  static Image* LoadImage(const std::string& path);
  static void UnloadImage(Image *img);

  static Font* LoadFont(const std::string& path);
  static void UnloadFont(Font* font);
  static Font* LoadSystemFont();

  template <typename T>
  static T* LoadObject(const std::string &path);
  template <typename T>
  static void UnloadObject(T* obj);

  static void UpdateMovie();

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
  static void SetAlias(const std::string& path_from, const std::string& path_to);

private:
  ResourceManager();
  ~ResourceManager();

  // cached paths
  std::vector<std::string> path_cached_;

  // path replacement
  std::map<std::string, std::string> path_replacement_;

  std::string ResolveMaskedPath(const std::string &path);
};

/* @brief Object from ResourceManager using RAII model */
template <typename T>
class ResourceObject
{
public:
  ResourceObject(T *obj)
    : obj_(obj) {};
  ResourceObject(const std::string &name)
    : obj_(ResourceManager::LoadObject<T>(name)) {};
  ~ResourceObject() { ResourceManager::UnloadObject<T>(obj); }
  T &get() const { return *obj_; }
  T &operator*() const { return *obj_; }
private:
  T *obj_;
};

}