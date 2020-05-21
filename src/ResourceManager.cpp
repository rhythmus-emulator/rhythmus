#include "ResourceManager.h"
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
class ResourceLoaderTask : public Task
{
public:
  ResourceLoaderTask(Image *img)
    : p_(0), len_(0), img_to_load_(img), font_to_load_(nullptr), sound_to_load_(nullptr) {}
  ResourceLoaderTask(Font *font)
    : p_(0), len_(0), img_to_load_(nullptr), font_to_load_(font), sound_to_load_(nullptr) {}
  ResourceLoaderTask(SoundData *sound)
    : p_(0), len_(0), img_to_load_(nullptr), font_to_load_(nullptr), sound_to_load_(sound) {}

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

  virtual void run()
  {
    std::lock_guard<std::mutex> resource_lock(gResourceLoadLock);

    if (img_to_load_)
    {
      if (p_ == nullptr)
        img_to_load_->Load(filename_);
      else
        img_to_load_->Load(p_, len_,
          filename_.empty() ? nullptr : filename_.c_str());
      img_to_load_->set_parent_task(nullptr);
    }
    else if (font_to_load_)
    {
      if (p_ == nullptr)
        font_to_load_->Load(filename_);
      else
        font_to_load_->Load(p_, len_,
          filename_.empty() ? nullptr : filename_.c_str());
      font_to_load_->set_parent_task(nullptr);
    }
    else if (sound_to_load_)
    {
      if (p_ == nullptr)
        sound_to_load_->Load(filename_);
      else
        sound_to_load_->Load(p_, len_,
          filename_.empty() ? nullptr : filename_.c_str());
      sound_to_load_->set_parent_task(nullptr);
    }
  }

  virtual void abort()
  {
    std::lock_guard<std::mutex> resource_lock(gResourceLoadLock);

    if (img_to_load_)
      img_to_load_->set_parent_task(nullptr);
    else if (font_to_load_)
      font_to_load_->set_parent_task(nullptr);
    else if (sound_to_load_)
      sound_to_load_->set_parent_task(nullptr);
    img_to_load_ = nullptr;
    font_to_load_ = nullptr;
    sound_to_load_ = nullptr;
  }

private:
  Image *img_to_load_;
  Font *font_to_load_;
  SoundData *sound_to_load_;

  const char *p_;
  size_t len_;
  std::string filename_;
};


ResourceElement::ResourceElement() : parent_task_(0), ref_count_(1) {}

ResourceElement::~ResourceElement() {}

void ResourceElement::set_name(const std::string &name)
{
  name_ = name;
}

const std::string &ResourceElement::get_name() const
{
  return name_;
}

void ResourceElement::set_parent_task(Task *task) { parent_task_ = task; }
Task *ResourceElement::get_parent_task() { return parent_task_; }

bool ResourceElement::is_loading() const
{
  return parent_task_ != nullptr;
}

void ResourceContainer::AddResource(ResourceElement *elem)
{
  elems_.push_back(elem);
}

ResourceElement* ResourceContainer::SearchResource(const std::string &name)
{
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
  auto ii = std::find(elems_.begin(), elems_.end(), elem);
  R_ASSERT(ii != elems_.end());
  elems_.erase(ii);
  if (elem->ref_count_ == 0)
  {
    std::lock_guard<std::mutex> resource_lock(gResourceLoadLock);
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
  if (path.empty()) return nullptr;
  Image *r = nullptr;
  std::string newpath = PATH->GetPath(path);
  r = (Image*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new Image();
    r->set_name(newpath);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(newpath);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
  return r;
}

Image* ImageManager::Load(const char *p, size_t len, const char *name_opt)
{
  Image *r = nullptr;
  if (name_opt)
    r = (Image*)SearchResource(name_opt);
  if (!r)
  {
    r = new Image();
    if (name_opt) r->set_name(name_opt);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(p, len, name_opt);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
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
  r = (SoundData*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new SoundData();
    r->set_name(newpath);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(newpath);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
  return r;
}

SoundData* SoundManager::Load(const char *p, size_t len, const char *name_opt)
{
  SoundData *r = nullptr;
  if (name_opt)
    r = (SoundData*)SearchResource(name_opt);
  if (!r)
  {
    r = new SoundData();
    if (name_opt) r->set_name(name_opt);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(p, len, name_opt);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
  return r;
}

void SoundManager::Unload(SoundData *sound)
{
  DropResource(sound);
}

void SoundManager::set_load_async(bool load_async) { load_async_ = load_async; }


// ---------------------------------------------------------- class FontManager

FontManager::FontManager() : load_async_(false) {}

FontManager::~FontManager()
{
  if (!is_empty())
    Logger::Error("FontManager has unreleased resources. memory leak!");
}

Font* FontManager::Load(const std::string &path)
{
  if (path.empty()) return nullptr;
  Font *r = nullptr;
  std::string newpath = PATH->GetPath(path);
  r = (Font*)SearchResource(newpath.c_str());
  if (!r)
  {
    r = new Font();
    r->set_name(newpath);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(newpath);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
  return r;
}

Font* FontManager::Load(const char *p, size_t len, const char *name_opt)
{
  Font *r = nullptr;
  if (name_opt)
    r = (Font*)SearchResource(name_opt);
  if (!r)
  {
    r = new Font();
    if (name_opt) r->set_name(name_opt);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(p, len, name_opt);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
  return r;
}

Font* FontManager::Load(const MetricGroup &metrics)
{
  Font *r = nullptr;
  std::string name;
  if (metrics.get_safe("name", name) && !name.empty())
    r = (Font*)SearchResource(name);
  if (!r)
  {
    r = new Font();
    if (!name.empty()) r->set_name(name);
    if (load_async_)
    {
      ResourceLoaderTask *task = new ResourceLoaderTask(r);
      r->set_parent_task(task);
      TaskPool::getInstance().EnqueueTask(task);
    }
    else
    {
      r->Load(metrics);
    }
    // register resource regardless it is succeed to load or not.
    AddResource(r);
  }
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
  }
}

void FontManager::set_load_async(bool load_async) { load_async_ = load_async; }


// ------------------------------------------------------ class ResourceManager

void ResourceManager::Initialize()
{
  PATH = new PathCache();
  IMAGEMAN = new ImageManager();
  SOUNDMAN = new SoundManager();
  FONTMAN = new FontManager();

  /* Cache system directory hierarchy. */
  PATH->CacheSystemDirectory();

#if 0
  // TODO: Create Font.System metrics for default system font.
  a.name = "System";
  a.SetPath("system/default.ttf");
  a.SetSize(5);
  a.color = 0xFFFFFFFF;
  gFontAttributes[a.name] = a;
#endif
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


PathCache *PATH = nullptr;
ImageManager *IMAGEMAN = nullptr;
SoundManager *SOUNDMAN = nullptr;
FontManager *FONTMAN = nullptr;

}
