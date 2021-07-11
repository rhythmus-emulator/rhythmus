#include "ResourceManager.h"
#include "Game.h"
#include "Image.h"
#include "Font.h"
#include "Sound.h"
#include "Timer.h"
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
    if (name_optional) filename_ = name_optional;
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

    if (obj_) {
      if (p_ != nullptr && len_ > 0)
        obj_->Load(p_, len_,
          filename_.empty() ? nullptr : filename_.c_str());
      else if (filename_.empty())
        obj_->Load(metric_);
      else
        obj_->Load(filename_);
      obj_->set_parent_task(nullptr);
      if (obj_->get_error_code() != 0) {
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
  : parent_task_(0), ref_count_(1), error_msg_(0), error_code_(0) {}

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
}

Task *ResourceElement::get_parent_task() { return parent_task_; }

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


// --------------------------------------------------------- class ImageManager

ImageManager::ImageManager() {}

ImageManager::~ImageManager()
{
  if (!is_empty())
    Logger::Error("ImageManager has unreleased resources. memory leak!");
}

Image* ImageManager::Load(const std::string& path)
{
  return Load(FilePath(path));
}

/* This function gurantees synchronized result. */
Image* ImageManager::Load(const FilePath& path)
{
  Image *r = nullptr;
  if (!path.valid()) return nullptr;
  lock_.lock();
  r = (Image*)SearchResource(path.get());
  if (!r) {
    r = new Image();
    r->set_name(path.get());
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    r->Load(path.get());
#if 0
    if (GAME->is_main_thread()) {
      r->Load(newpath);
    }
    else {
      auto* task = new ResourceLoaderTask<Image>(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TASKMAN->Await(task);
    }
#endif
  }
  else lock_.unlock();
  return r;
}

Image* ImageManager::Load(const char *p, size_t len, const char *name_opt)
{
  Image *r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (Image*)SearchResource(name_opt);
  if (!r) {
    r = new Image();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    r->Load(p, len, name_opt);
#if 0
    if (GAME->is_main_thread()) {
      r->Load(p, len, name_opt);
    }
    else {
      auto* task = new ResourceLoaderTask<Image>(r);
      r->set_parent_task(task);
      task->SetData(p, len, name_opt);
      TASKMAN->Await(task);
    }
#endif
  }
  else lock_.unlock();
  return r;
}

Image* ImageManager::LoadAsync(const FilePath& path, ITaskCallback *callback)
{
  Image* r = nullptr;
  if (!path.valid()) return nullptr;
  lock_.lock();
  r = (Image*)SearchResource(path.get());
  if (!r) {
    r = new Image();
    r->set_name(path.get());
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    auto* task = new ResourceLoaderTask<Image>(r);
    task->set_callback(callback);
    task->SetFilename(path.get());
    r->set_parent_task(task);
    TASKMAN->Await(task);
  }
  else {
    // already loaded or being loaded.
    // instantly run callback and return object.
    lock_.unlock();
    if (callback) callback->run();
  }
  return r;
}

Image* ImageManager::LoadAsync(const char* p, size_t len, const char* name_opt, ITaskCallback *callback)
{
  Image* r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (Image*)SearchResource(name_opt);
  if (!r) {
    r = new Image();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    auto* task = new ResourceLoaderTask<Image>(r);
    task->set_callback(callback);
    task->SetData(p, len, name_opt);
    r->set_parent_task(task);
    TASKMAN->Await(task);
  }
  else {
    // already loaded or being loaded.
    // instantly run callback and return object.
    lock_.unlock();
    if (callback) callback->run();
  }
  return r;
}

void ImageManager::Unload(Image *image)
{
  /* WARN: must be called at main thread */
  DropResource(image);
}

void ImageManager::Update(double ms)
{
  /* update images; e.g. Refresh movie bitmap or uploading texture */
  /* WARN: must be called at main thread */
  for (auto *e : *this) {
    ((Image*)e)->Update(ms);
    if (e->get_error_code()) {
      Logger::Error("Image object error: %s (%d)",
        e->get_error_msg(), e->get_error_code());
      e->clear_error();
    }
  }
}


// --------------------------------------------------------- class SoundManager

SoundManager::SoundManager() {}

SoundManager::~SoundManager()
{
  if (!is_empty())
    Logger::Error("SoundManager has unreleased resources. memory leak!");
}

SoundData* SoundManager::Load(const std::string& path)
{
  return Load(FilePath(path));
}

SoundData* SoundManager::Load(const FilePath& path)
{
  if (!path.valid()) return nullptr;
  SoundData *r = nullptr;
  lock_.lock();
  r = (SoundData*)SearchResource(path.get());
  if (!r) {
    r = new SoundData();
    r->set_name(path.get());
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    // Sound is always loaded as async.
    auto *task = new ResourceLoaderTask<SoundData>(r);
    task->SetFilename(path.get());
    r->set_parent_task(task);
    TASKMAN->EnqueueTask(task);
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
    // Sound is always loaded as async.
    auto* task = new ResourceLoaderTask<SoundData>(r);
    task->SetData(p, len, name_opt);
    r->set_parent_task(task);
    TASKMAN->EnqueueTask(task);
  }
  lock_.unlock();
  return r;
}

SoundData* SoundManager::LoadAsync(const FilePath& path, ITaskCallback* callback)
{
  SoundData* r = nullptr;
  if (!path.valid()) return nullptr;
  lock_.lock();
  r = (SoundData*)SearchResource(path.get());
  if (!r) {
    r = new SoundData();
    r->set_name(path.get());
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    auto* task = new ResourceLoaderTask<SoundData>(r);
    task->set_callback(callback);
    task->SetFilename(path.get());
    r->set_parent_task(task);
    TASKMAN->Await(task);
  }
  else {
    // already loaded or being loaded.
    // instantly run callback and return object.
    lock_.unlock();
    if (callback) callback->run();
  }
  return r;
}

SoundData* SoundManager::LoadAsync(const char* p, size_t len, const char* name_opt, ITaskCallback* callback)
{
  SoundData* r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (SoundData*)SearchResource(name_opt);
  if (!r) {
    r = new SoundData();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    auto* task = new ResourceLoaderTask<SoundData>(r);
    task->set_callback(callback);
    task->SetData(p, len, name_opt);
    r->set_parent_task(task);
    TASKMAN->Await(task);
  }
  else {
    // already loaded or being loaded.
    // instantly run callback and return object.
    lock_.unlock();
    if (callback) callback->run();
  }
  return r;
}

void SoundManager::Unload(SoundData *sound)
{
  DropResource(sound);
}


// ---------------------------------------------------------- class FontManager

FontManager::FontManager() {}

FontManager::~FontManager()
{
  if (!is_empty())
    Logger::Error("FontManager has unreleased resources. memory leak!");
}

Font* FontManager::Load(const std::string& path)
{
  return Load(FilePath(path));
}

Font* FontManager::Load(const FilePath& path)
{
  if (!path.valid()) return nullptr;
  Font *r = nullptr;
  lock_.lock();
  r = (Font*)SearchResource(path.get());
  if (!r) {
    r = new Font();
    r->set_name(path.get());
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    r->Load(path.get());
#if 0
    if (GAME->is_main_thread()) {
      r->Load(newpath);
    }
    else {
      auto* task = new ResourceLoaderTask<Font>(r);
      task->SetFilename(newpath);
      r->set_parent_task(task);
      TASKMAN->Await(task);
    }
#endif
  }
  else lock_.unlock();
  return r;
}

Font* FontManager::Load(const char *p, size_t len, const char *name_opt)
{
  Font *r = nullptr;
  lock_.lock();
  if (name_opt)
    r = (Font*)SearchResource(name_opt);

  if (!r) {
    r = new Font();
    if (name_opt) r->set_name(name_opt);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    r->Load(p, len, name_opt);
#if 0
    if (GAME->is_main_thread()) {
      r->Load(p, len, name_opt);
    }
    else {
      auto* task = new ResourceLoaderTask<Font>(r);
      task->SetData(p, len, name_opt);
      r->set_parent_task(task);
      TASKMAN->Await(task);
    }
#endif
  }
  else lock_.unlock();
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
  if (!r) {
    r = new Font();
    if (!name.empty()) r->set_name(name);
    // register resource first regardless it is succeed to load or not.
    AddResource(r);
    lock_.unlock();
    r->Load(metrics);
#if 0
    if (GAME->is_main_thread()) {
      r->Load(metrics);
    }
    else {
      auto* task = new ResourceLoaderTask<Font>(r);
      task->SetMetric(metrics);
      r->set_parent_task(task);
      TASKMAN->Await(task);
    }
#endif
  }
  else lock_.unlock();
  return r;
}

void FontManager::Unload(Font *font)
{
  DropResource(font);
}

void FontManager::Update(double ms)
{
  /* update font object */
  for (auto *e : *this) {
    ((Font*)e)->Update((float)ms);
    if (e->get_error_code()) {
      Logger::Error("Font object error: %s (%d)",
        e->get_error_msg(), e->get_error_code());
      e->clear_error();
    }
  }
}

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

Font* FontManager::GetSystemFont()
{
  static Font* sysfont = nullptr;
  if (sysfont == NULL) {
    MetricGroup *sysfontmetric = METRIC->get_group("SystemFont");
    R_ASSERT(sysfontmetric);
    sysfont = FONTMAN->Load(*sysfontmetric);
  }
  return sysfont;
}

// ------------------------------------------------------ class ResourceManager

void ResourceManager::Initialize()
{
  IMAGEMAN = new ImageManager();
  SOUNDMAN = new SoundManager();
  FONTMAN = new FontManager();
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


ImageManager *IMAGEMAN = nullptr;
SoundManager *SOUNDMAN = nullptr;
FontManager *FONTMAN = nullptr;

}
