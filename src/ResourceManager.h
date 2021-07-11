#pragma once

#include "TaskPool.h"
#include "Util.h"
#include "Path.h"
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
  ResourceElement *clone() const;

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
  mutable int ref_count_;

protected:
  const char* error_msg_;
  int error_code_;
};

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

/* @brief Image object manager. */
class ImageManager : public ResourceContainer
{
public:
  ImageManager();
  virtual ~ImageManager();
  Image* Load(const std::string& path);
  Image* Load(const FilePath& path);
  Image* Load(const char *p, size_t len, const char *name_opt);
  Image* LoadAsync(const FilePath& path, ITaskCallback* callback);
  Image* LoadAsync(const char* p, size_t len, const char* name_opt, ITaskCallback* callback);
  void Unload(Image *image);
  void Update(double ms);
  
private:
  std::mutex lock_;
};

/* Sound object manager.
 * @warn Sound is always loaded in async. */
class SoundManager : private ResourceContainer
{
public:
  SoundManager();
  ~SoundManager();
  SoundData* Load(const std::string& path);
  SoundData* Load(const FilePath& path);
  SoundData* Load(const char *p, size_t len, const char *name_opt);
  SoundData* LoadAsync(const FilePath& path, ITaskCallback* callback);
  SoundData* LoadAsync(const char* p, size_t len, const char* name_opt, ITaskCallback* callback);
  void Unload(SoundData *sound);

private:
  std::mutex lock_;
};

/* @brief Font object manager. */
class FontManager : private ResourceContainer
{
public:
  FontManager();
  ~FontManager();
  Font* Load(const std::string& path);
  Font* Load(const FilePath& path);
  Font* Load(const char *p, size_t len, const char *name_opt);
  Font* Load(const MetricGroup &metrics);
  void Unload(Font *font);
  void Update(double ms);

  static void SetSystemFont();
  static Font* GetSystemFont();

private:
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

private:
  ResourceManager() {};
};

extern ImageManager *IMAGEMAN;
extern SoundManager *SOUNDMAN;
extern FontManager *FONTMAN;

}
