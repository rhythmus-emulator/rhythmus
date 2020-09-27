#include "ResourceManager.h"
#include "Game.h"
#include "Image.h"
#include "Font.h"
#include "Sound.h"
#include "Timer.h"
#include "TaskPool.h"
#include "Logger.h"
#include "Util.h"
#include "common.h"
#include <FreeImage.h>

namespace rhythmus
{

/*
 * @brief
 * Global lock for object loading from ResourceManager.
 * Must used when object is being removed to cancel loading task
 * before it started.
 */
std::mutex gResourceLoadLock;

/* @brief Loader thread. */
template <typename T>
class ResourceLoaderTask : public Task
{
public:
  ResourceLoaderTask(T *obj)
    : obj_(obj), p_(0), len_(0) {}

  void SetData(const char *p, size_t len, const char *name_optional)
  {
    p_ = p;
    len_ = len;
    filename_ = name_optional;
  }

  void SetFilename(const std::string &filename)
  {
    p_ = nullptr;
    len_ = 0;
    filename_ = filename;
  }

  void SetMetric(const MetricGroup &m)
  {
    p_ = nullptr;
    len_ = 0;
    filename_.clear();
    metric_ = m;
  }

  virtual void run()
  {
    std::lock_guard<std::mutex> resource_lock(gResourceLoadLock);

    if (obj_)
    {
      if (p_ != nullptr && len_ > 0)
        obj_->Load(p_, len_,
          filename_.empty() ? nullptr : filename_.c_str());
      else if (filename_.empty())
        obj_->Load(metric_);
      else
        obj_->Load(filename_);
      obj_->set_parent_task(nullptr);
      if (obj_->get_error_code() != 0)
      {
        Logger::Error("Cannot read image %s: %s (%d)",
            filename_.c_str(), obj_->get_error_msg(),
          obj_->get_error_code());
        obj_->clear_error();
      }
    }
  }

  virtual void abort()
  {
    std::lock_guard<std::mutex> resource_lock(gResourceLoadLock);
    if (obj_)
      obj_->set_parent_task(nullptr);
    obj_ = nullptr;
  }

private:
  T *obj_;
  const char *p_;
  size_t len_;
  std::string filename_;
  MetricGroup metric_;
};


ResourceElement::ResourceElement()
  : parent_task_(0), ref_count_(1), is_loading_(false), error_msg_(0), error_code_(0) {}

ResourceElement::~ResourceElement() {}

void ResourceElement::set_name(const std::string &name)
{
  name_ = name;
}

const std::string &ResourceElement::get_name() const
{
  return name_;
}

void ResourceElement::set_parent_task(Task *task)
{
  parent_task_ = task;
  is_loading_ = (task != nullptr);
}

Task *ResourceElement::get_parent_task() { return parent_task_; }

bool ResourceElement::is_loading() const
{
  // parent_task_ : loader task used by async load method.
  // is_loading_ : flag used by sync load method
  return parent_task_ != nullptr || is_loading_;
}

ResourceElement *ResourceElement::clone() const
{
  ref_count_++;
  return const_cast<ResourceElement*>(this);
}

const char *ResourceElement::get_error_msg() const
{
  return error_msg_;
}

int ResourceElement::get_error_code() const
{
  return error_code_;
}

void ResourceElement::clear_error()
{
  error_msg_ = 0;
  error_code_ = 0;
}

// @DEPRECIATED
// This function must be called when an object need to be loaded for sure
// Even if ResourceContainer::load_async is true, because shared object
// can be fetch although it isn't loaded.
// (actually being loaded by other thread)
void SleepUntilLoadFinish(const ResourceElement *e)
{
  if (!e) return;
  while (e->is_loading()) Sleep(1);
}

void ResourceContainer::AddResource(ResourceElement *elem)
{
  std::lock_guard<std::mutex> l(lock_);
  elems_.push_back(elem);
}

ResourceElement* ResourceContainer::SearchResource(const std::string &name)
{
  std::lock_guard<std::mutex> l(lock_);
  if (!name.empty())
  {
    for (auto *e : elems_)
    {
      if (e->name_ == name)
      {
        e->ref_count_++;
        return e;
      }
    }
  }
  return nullptr;
}

void ResourceContainer::DropResource(ResourceElement *elem)
{
  std::lock_guard<std::mutex> l(lock_);
  auto ii = std::find(elems_.begin(), elems_.end(), elem);
  R_ASSERT(ii != elems_.end());
  if (--elem->ref_count_ == 0)
  {
    elems_.erase(ii);
    auto *t = elem->get_parent_task();
    if (t) t->abort();
    delete elem;
  }
}

ResourceContainer::iterator ResourceContainer::begin() { return elems_.begin(); }
ResourceContainer::iterator ResourceContainer::end() { return elems_.end(); }
bool ResourceContainer::is_empty() const { return elems_.empty(); }



// ------------------------------------------------------------ class PathCache

PathCache::PathCache() {}

void PathCache::CacheSystemDirectory()
{
  /* cache system paths for resource. (not song path) */
  CacheDirectory("themes/");
  CacheDirectory("sound/");
  CacheDirectory("bgm/"); /* for LR2 compatiblity */
}

void PathCache::CacheDirectory(const std::string& dir)
{
  // XXX: currently only files are cached.
  GetFilesFromDirectory(dir, path_cached_, 500 /* not so deep ...? */);
}

void PathCache::MakePathHigherPriority(const std::string& path)
{
  auto it = std::find(path_cached_.begin(), path_cached_.end(), path);
  if (it == path_cached_.end()) return;
  std::rotate(path_cached_.begin(), it, path_cached_.end());
}

void PathCache::SetPrefixReplace(const std::string &prefix_from,
                                 const std::string &prefix_to)
{
  path_prefix_replace_from_ = prefix_from;
  path_prefix_replace_to_ = prefix_to;
}

void PathCache::ClearPrefixReplace()
{
  path_prefix_replace_from_.clear();
  path_prefix_replace_to_.clear();
}

std::string PathCache::GetPath(const std::string& masked_path,
                               bool &is_found) const
{
  is_found = false;

  // attempt to find path replacement first ...
  auto it = path_replacement_.find(masked_path);
  if (it != path_replacement_.end())
  {
    is_found = true;
    return it->second;
  }

  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return PrefixReplace(masked_path);

  // do path mask matching for cached paths
  std::string r = ResolveMaskedPath(masked_path);
  if (r.empty())
  {
    is_found = false;
    return PrefixReplace(masked_path);
  }
  return r;
}

std::string PathCache::GetPath(const std::string& masked_path) const
{
  bool _;
  return GetPath(masked_path, _);
}

void PathCache::GetAllPaths(const std::string& masked_path, std::vector<std::string> &out) const
{
  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return;

  // do path mask matching for cached paths
  for (const auto& path : path_cached_)
  {
    if (CheckMasking(path, masked_path))
    {
      out.push_back(PrefixReplace(path));
    }
  }
}

void PathCache::SetAlias(const std::string& path_from, const std::string& path_to)
{
  // check masked path. if so, resolve it.
  path_replacement_[path_from] = ResolveMaskedPath(path_to);
}

std::string PathCache::ResolveMaskedPath(const std::string &masked_path) const
{
  // check masking first -- if not, return as it is.
  if (masked_path.find('*') == std::string::npos)
    return PrefixReplace(masked_path);

  // do path mask matching for cached paths
  std::string replaced_path = PrefixReplace(masked_path);
  for (const auto& path : path_cached_)
  {
    if (CheckMasking(path, replaced_path))
    {
      return path;
    }
  }

  // if not found, return empty string
  return std::string();
}

std::string PathCache::PrefixReplace(const std::string &path) const
{
  std::string newpath = path;
  for (size_t i = 0; i < newpath.size(); ++i)
    if (newpath[i] == '\\') newpath[i] = '/';
  if (startsWith(newpath, "./"))
    newpath = newpath.substr(2);
  if (path_prefix_replace_from_.empty()) return newpath;
  if (strnicmp(
    path_prefix_replace_from_.c_str(),
    newpath.c_str(), path_prefix_replace_from_.size()) == 0)
  {
    return path_prefix_replace_to_ + newpath.substr(path_prefix_replace_from_.size());
  }
  else return newpath;
}


// --------------------------------------------------------- class ImageManager

ImageManager::ImageManager() : load_async_(true) {}

ImageManager::~ImageManager()
{
  if (!is_empty())
    Logger::Error("ImageManager has unreleased resources. memory leak!");
}

Image* ImageManager::Load(const std::string &path)
{
  Image *r = nullptr;
  std::string newpath = PATH->GetPath(path);
  if (newpath.empty()) return nullptr;
  lock_.lock();
  r = (Image*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new Image();
    r->set_name(newpath);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<Image>(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(newpath);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

Image* ImageManager::Load(const char *p, size_t len, const char *name_opt)
{
  Image *r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (Image*)SearchResource(name_opt);
  if (!r)
  {
    r = new Image();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<Image>(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(p, len, name_opt);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

void ImageManager::Unload(Image *image)
{
  DropResource(image);
}

void ImageManager::Update(double ms)
{
  for (auto *e : *this)
  {
    ((Image*)e)->Update(ms);
    if (e->get_error_code())
    {
      Logger::Error("Image object error: %s (%d)",
        e->get_error_msg(), e->get_error_code());
      e->clear_error();
    }
  }
}

void ImageManager::set_load_async(bool load_async) { load_async_ = load_async; }


// --------------------------------------------------------- class SoundManager

SoundManager::SoundManager() : load_async_(false) {}

SoundManager::~SoundManager()
{
  if (!is_empty())
    Logger::Error("SoundManager has unreleased resources. memory leak!");
}

SoundData* SoundManager::Load(const std::string &path)
{
  if (path.empty()) return nullptr;
  SoundData *r = nullptr;
  std::string newpath = PATH->GetPath(path);
  lock_.lock();
  r = (SoundData*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new SoundData();
    r->set_name(newpath);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<SoundData>(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(newpath);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

SoundData* SoundManager::Load(const char *p, size_t len, const char *name_opt)
{
  SoundData *r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (SoundData*)SearchResource(name_opt);
  if (!r)
  {
    r = new SoundData();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<SoundData>(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(p, len, name_opt);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

void SoundManager::Unload(SoundData *sound)
{
  DropResource(sound);
}

void SoundManager::set_load_async(bool load_async) { load_async_ = load_async; }


// ---------------------------------------------------------- class FontManager

FontManager::FontManager() : load_async_(true) {}

FontManager::~FontManager()
{
  if (!is_empty())
    Logger::Error("FontManager has unreleased resources. memory leak!");
}

Font* FontManager::Load(const std::string &path)
{
  if (path.empty()) return nullptr;
  Font *r = nullptr;
  lock_.lock();
  std::string newpath = PATH->GetPath(path);
  r = (Font*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new Font();
    r->set_name(newpath);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<Font>(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(newpath);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

Font* FontManager::Load(const char *p, size_t len, const char *name_opt)
{
  Font *r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (Font*)SearchResource(name_opt);

  if (!r)
  {
    r = new Font();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);

    /* if not main thread, it's still "async";
      * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<Font>(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(p, len, name_opt);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

Font* FontManager::Load(const MetricGroup &metrics)
{
  Font *r = nullptr;
  std::string name;
  lock_.lock();
  if (metrics.get_safe("name", name) && !name.empty())
    r = (Font*)SearchResource(name);
  if (!r && metrics.get_safe("path", name) && !name.empty())
    r = (Font*)SearchResource(name);
  if (!r)
  {
    r = new Font();
    if (!name.empty()) r->set_name(name);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    /* if not main thread, it's still "async";
     * don't need to create other thread. */
    if (load_async_ && GAME->is_main_thread())
    {
      auto *task = new ResourceLoaderTask<Font>(r);
      task->SetMetric(metrics);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->is_loading_ = true;
      // unlock here first, as this method takes long
      lock_.unlock();
      r->Load(metrics);
      lock_.lock();
      r->is_loading_ = false;
    }
  }
  lock_.unlock();
  return r;
}

void FontManager::Unload(Font *font)
{
  DropResource(font);
}

void FontManager::Update(double ms)
{
  for (auto *e : *this)
  {
    ((Font*)e)->Update((float)ms);
    if (e->get_error_code())
    {
      Logger::Error("Font object error: %s (%d)",
        e->get_error_msg(), e->get_error_code());
      e->clear_error();
    }
  }
}

void FontManager::set_load_async(bool load_async) { load_async_ = load_async; }

void FontManager::SetSystemFont()
{
  MetricGroup &sysfont = METRIC->add_group("SystemFont");
  sysfont.set("name", "SystemFont");
  sysfont.set("path", "system/default.ttf");
  sysfont.set("size", 16);
  sysfont.set("color", "#FFFFFFFF");
  sysfont.set("border-size", 1);
  sysfont.set("border-color", "#FF000000");
}

// ------------------------------------------------------ class ResourceManager

void ResourceManager::Initialize()
{
  PATH = new PathCache();
  IMAGEMAN = new ImageManager();
  SOUNDMAN = new SoundManager();
  FONTMAN = new FontManager();

  /* Cache system directory hierarchy. */
  PATH->CacheSystemDirectory();
}

void ResourceManager::Cleanup()
{
  delete IMAGEMAN;
  delete SOUNDMAN;
  delete FONTMAN;
  delete PATH;

  PATH = nullptr;
  IMAGEMAN = nullptr;
  SOUNDMAN = nullptr;
  FONTMAN = nullptr;
}

void ResourceManager::Update(double ms)
{
  IMAGEMAN->Update(ms);
  FONTMAN->Update(ms);
}

bool ResourceManager::IsLoading()
{
  return TaskPool::getInstance().is_idle() == false;
}


PathCache *PATH = nullptr;
ImageManager *IMAGEMAN = nullptr;
SoundManager *SOUNDMAN = nullptr;
FontManager *FONTMAN = nullptr;

}
