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
  /* Load image from given path. */
  static Image* LoadImage(const std::string &path);
  static void UnloadImage(Image *img);

  /* Load font from given path.
   * @warn
   * Font is not ttf file; it should be font spec file.
   * If ttf file specified, then default font caching option will be applied. */
  static Font* LoadFont(const std::string &path);
  static Font* LoadFont(const FontAttributes &attr);
  /* Load font by name. The font should have been cached previously. */
  static Font* LoadFontByName(const std::string &name);
  static Font* LoadSystemFont();
  static void UnloadFont(Font* font);

  /* @brief Read font attribute from path. Necessary if font should be loaded by name. */
  const FontAttributes* ReadFontAttribute(const std::string &path);
  const FontAttributes* GetFontAttributeByName(const std::string &name);
  void ClearFontAttribute();

  template <typename T>
  static T* LoadObject(const std::string &path);
  template <typename T>
  static void UnloadObject(T* obj);

  static void UpdateMovie();

  static ResourceManager& getInstance();

  static void CacheSystemDirectory();

  /**
   * @brief
   * Initalize directory hierarchery and default resource profile.
   */
  void Initialize();

  /**
   * @brief
   * check resource states and deallocates all resource objects.
   */
  void Cleanup();

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
   * Use when resource path need to be replaced.
   * (for LR2)
   */
  void SetPrefixReplace(const std::string &prefix_from, const std::string &prefix_to);
  void ClearPrefixReplace();

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

  // path prefix replacement
  std::string path_prefix_replace_from_;
  std::string path_prefix_replace_to_;

  std::string ResolveMaskedPath(const std::string &path) const;
  std::string PrefixReplace(const std::string &path) const;
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
  ~ResourceObject() { ResourceManager::UnloadObject<T>(obj_); }
  T &get() const { return *obj_; }
  T &operator*() const { return *obj_; }
private:
  T *obj_;
};

}
