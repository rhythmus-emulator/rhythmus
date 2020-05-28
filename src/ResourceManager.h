#pragma once

#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

namespace rhythmus
{

class Task;
class MetricGroup;
class Image;
class Font;
class SoundData;

/* @brief All resource objects, which is async-loadable. */
class ResourceElement
{
public:
  ResourceElement();
  virtual ~ResourceElement();
  void set_name(const std::string &name);
  const std::string &get_name() const;
  void set_parent_task(Task *task);
  Task *get_parent_task();
  bool is_loading() const;

  const char *get_error_msg() const;
  int get_error_code() const;
  void clear_error();

  friend class ResourceContainer;
  friend class ImageManager;
  friend class FontManager;
  friend class SoundManager;

private:
  Task *parent_task_;
  std::string name_;
  int ref_count_;
  bool is_loading_;

protected:
  const char* error_msg_;
  int error_code_;
};

void SleepUntilLoadFinish(const ResourceElement *e);

/* @brief Base container for ResourceElement. */
class ResourceContainer
{
public:
  void AddResource(ResourceElement *elem);
  ResourceElement* SearchResource(const std::string &name);
  void DropResource(ResourceElement *elem);

  typedef std::list<ResourceElement*>::iterator iterator;
  iterator begin();
  iterator end();
  bool is_empty() const;
  
private:
  std::list<ResourceElement*> elems_;
  std::mutex lock_;
};

/* @brief Cache directory/files managed by program. */
class PathCache
{
public:
  PathCache();

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
  void SetAlias(const std::string& path_from, const std::string& path_to);

  /**
   * @brief
   * Cache a folder and it's subdirectory path.
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

  /**
   * @brief
   * Use when resource path need to be replaced.
   * (for LR2)
   */
  void SetPrefixReplace(const std::string &prefix_from, const std::string &prefix_to);
  void ClearPrefixReplace();

private:
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

/* @brief Image object manager. */
class ImageManager : private ResourceContainer
{
public:
  ImageManager();
  virtual ~ImageManager();
  Image* Load(const std::string &path);
  Image* Load(const char *p, size_t len, const char *name_opt);
  void Unload(Image *image);
  void Update(double ms);
  void set_load_async(bool load_async);
  
private:
  bool load_async_;
  std::mutex lock_;
};

/* @brief Font object manager. */
class SoundManager : private ResourceContainer
{
public:
  SoundManager();
  ~SoundManager();
  SoundData* Load(const std::string &path);
  SoundData* Load(const char *p, size_t len, const char *name_opt);
  void Unload(SoundData *sound);
  void set_load_async(bool load_async);

private:
  bool load_async_;
  std::mutex lock_;
};

/* @brief Font object manager. */
class FontManager : private ResourceContainer
{
public:
  FontManager();
  ~FontManager();
  Font* Load(const std::string &path);
  Font* Load(const char *p, size_t len, const char *name_opt);
  Font* Load(const MetricGroup &metrics);
  void Unload(Font *font);
  void Update(double ms);
  void set_load_async(bool load_async);

private:
  bool load_async_;
  std::mutex lock_;
};


class ResourceManager
{
public:
  /**
   * @brief
   * Initalize directory hierarchery and default resource profile.
   */
  static void Initialize();

  /**
   * @brief
   * check resource states and deallocates all resource objects.
   */
  static void Cleanup();

  /**
   * @brief
   * Update status of registered resource.
   */
  static void Update(double ms);

  static bool IsLoading();

private:
  ResourceManager() {};
};


extern PathCache *PATH;
extern ImageManager *IMAGEMAN;
extern SoundManager *SOUNDMAN;
extern FontManager *FONTMAN;

}
